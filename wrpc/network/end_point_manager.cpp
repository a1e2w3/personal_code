/**
 * @file end_point_manager.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 14:54:54
 * @brief 
 *  
 **/
 
#include "network/end_point_manager.h"

#include <functional>
#include <vector>

#include "utils/common_define.h"
 
namespace wrpc {

static ConnectionPtr new_connection(const EndPoint& end_point, int32_t timeout_ms) {
    ConnectionPtr connection(new Connection(end_point));
    int ret = connection->connect(timeout_ms);
    if (ret != NET_SUCC) {
        connection.reset();
    }
    return std::move(connection);
}

void EndPointManager::EndPointWrapper::create_connect_pool(size_t max_size) {
    if (!conn_pool) {
        conn_pool.reset(new ConnectionPool(std::bind(new_connection, end_point, std::placeholders::_1), max_size));
    }
}

void EndPointManager::EndPointWrapper::set_death() {
    status = DEATH;
}
void EndPointManager::EndPointWrapper::set_alive() {
    status = NORMAL;
    err_count = 0;
}
 
EndPointManager::EndPointManager()
    : _connection_type(SHORT), _max_error_count(-1), _conn_pool_max_size(1) {}
 
EndPointManager::~EndPointManager() {}
 
void EndPointManager::on_update(const EndPointList& endpoint_list) {
    std::vector<EndPoint> add_end_points;
    std::vector<EndPoint> del_end_points;
    size_t del_count = 0;
    size_t add_count = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // generate del patch
        for (const auto &pair : _end_points) {
            if (endpoint_list.find(pair.first) == endpoint_list.end()) {
                del_end_points.push_back(pair.first);
            }
        }

        // apply add patch
        for (const EndPoint& end_point : endpoint_list) {
            if (add_endpoint(end_point)) {
                add_end_points.push_back(end_point);
                ++add_count;
            }
        }

        // apply del patch
        for (const EndPoint& end_point : del_end_points) {
            if (del_endpoint(end_point)) {
                ++del_count;
            }
        }
    }

    // notify update after release lock
    if (add_count + del_count > 0) {
        if (add_count + del_count <= 2) {
            for (const EndPoint& ep : add_end_points) {
                notify_add_one(ep);
            }
            for (const EndPoint& ep : del_end_points) {
                notify_remove_one(ep);
            }
        } else {
            notify_update_all(endpoint_list);
        }
    }
}

void EndPointManager::health_check(int32_t connect_timeout_ms) {
    std::vector<EndPoint> dead_end_points;
    std::vector<EndPoint> alive_end_points;
    {
        // copy dead end points under read lock
        std::lock_guard<std::mutex> lock(_mutex);
        dead_end_points.reserve(_dead_end_points.size());
        dead_end_points.insert(dead_end_points.end(),
                _dead_end_points.begin(), _dead_end_points.end());
    }

    // check without lock to avoid blocking
    for (const EndPoint& dead_end_point : dead_end_points) {
        ConnectionPtr connection(
                new_connection(dead_end_point, connect_timeout_ms));
        if (connection) {
            // connection success, add to alive end point
            alive_end_points.push_back(dead_end_point);
        }
        connection.reset();
    }

    if (!alive_end_points.empty()) {
        // set alive under write lock
        std::lock_guard<std::mutex> lock(_mutex);
        for (const EndPoint& end_point : alive_end_points) {
            set_endpoint_alive(end_point);
        }
    }

    // notify alive after release lock
    for (const EndPoint& end_point : alive_end_points) {
        notify_set_alive(end_point);
    }
}

ConnectionPtr EndPointManager::fetch_connection(const EndPoint& end_point, int32_t timeout_ms) {
    ConnectionPtr connection;
    bool set_death = false;
    bool set_alive = false;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto iter = _end_points.find(end_point);
        if (iter == _end_points.end()) {
            // end point not found, just try to connect
            return new_connection(end_point, timeout_ms);
        }

        EndPointWrapper& wrapper = iter->second;
        //if (wrapper.status == DEATH) {
        //    // dead end point
        //    return ConnectionPtr();
        //}

        if (wrapper.conn_pool) {
            connection = wrapper.conn_pool->fetch(timeout_ms);
        } else {
            connection = new_connection(end_point, timeout_ms);
        }

        if (!connection) {
            // increase and check error count
            ++(wrapper.err_count);
            if (!check_error_count(wrapper.err_count)) {
                set_death = true;
                set_endpoint_death(end_point);
            }
        } else {
            // reset error count
            wrapper.err_count = 0;
            if (wrapper.status == DEATH) {
                // set alive
                _dead_end_points.erase(end_point);
                wrapper.set_alive();
                if (_connection_type == POOLED) {
                    wrapper.create_connect_pool(_conn_pool_max_size);
                }
                set_alive = true;
            }
        }
    }

    // notify after release lock
    if (set_death) {
        notify_set_death(end_point);
    } else if (set_alive) {
        notify_set_alive(end_point);
    }
    return std::move(connection);
}

void EndPointManager::giveback_connection(const EndPoint& end_point, ConnectionPtr&& connection, bool close) {
    if (close || !connection) {
        // close connection directly
        connection.reset();
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _end_points.find(end_point);
    if (iter == _end_points.end()) {
        // end point not found, close directly
        connection.reset();
        return;
    }

    EndPointWrapper& wrapper = iter->second;
    if (wrapper.status == DEATH) {
        // dead end point, close directly
        connection.reset();
        return;
    }

    if (wrapper.conn_pool) {
        // give back to connect pool
        wrapper.conn_pool->give_back(std::move(connection));
    } else {
        // close directly
        connection.reset();
    }
    return;
}

bool EndPointManager::add_endpoint(const EndPoint& end_point) {
    // will not set alive if end point exists and has been set death
    auto insert_ret = _end_points.emplace(end_point, EndPointWrapper(end_point));
    if (insert_ret.second && _connection_type == POOLED) {
        insert_ret.first->second.create_connect_pool(_conn_pool_max_size);
    }
    return insert_ret.second;
}

bool EndPointManager::del_endpoint(const EndPoint& end_point) {
    _dead_end_points.erase(end_point);
    return _end_points.erase(end_point) > 0;
}

void EndPointManager::set_endpoint_death(const EndPoint& end_point) {
    _dead_end_points.insert(end_point);
    auto iter = _end_points.find(end_point);
    if (iter != _end_points.end()) {
        iter->second.set_death();
        iter->second.release_connect_pool();
    }
}
void EndPointManager::set_endpoint_alive(const EndPoint& end_point) {
    _dead_end_points.erase(end_point);
    auto iter = _end_points.find(end_point);
    if (iter != _end_points.end()) {
        iter->second.set_alive();
        if (_connection_type == POOLED) {
            iter->second.create_connect_pool(_conn_pool_max_size);
        }
    }
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

