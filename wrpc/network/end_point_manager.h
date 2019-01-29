/**
 * @file end_point_manager.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 14:54:51
 * @brief 
 *      ∫Û∂Àendpointπ‹¿Ì∆˜
 **/
 
#ifndef WRPC_NETWORK_END_POINT_MANAGER_H_
#define WRPC_NETWORK_END_POINT_MANAGER_H_
 
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "common/end_point.h"
#include "common/options.h"
#include "interface/end_point_update_observer.h"
#include "interface/end_point_watcher.h"
#include "network/connection.h"
#include "utils/common_flags.h"
 
namespace wrpc {

enum EndPointStatus {
    NORMAL = 0,
    DEATH,
};

class EndPointManager : public IEndPointNotifier, public EndPointUpdateObserver {
public:
    EndPointManager();
    ~EndPointManager();

    virtual void on_update(const EndPointList& endpoint_list);

    ConnectionPtr fetch_connection(const EndPoint& end_point, int32_t timeout_ms);

    void giveback_connection(const EndPoint& end_point, ConnectionPtr&& connection, bool close);

    void health_check(int32_t connect_timeout_ms = WRPC_CONNECT_TIMEOUT_FOR_HEALTH_CHECK);

    void set_connection_type(ConnectionType type) {
        _connection_type = type;
    }

    ConnectionType get_connection_type() const {
        return _connection_type;
    }

    void set_max_error_count(int32_t max_error_count) {
        _max_error_count = max_error_count;
    }

    int32_t get_max_error_count() const {
        return _max_error_count;
    }

    void set_connect_pool_capacity(size_t conn_pool_capacity) {
        _conn_pool_max_size = conn_pool_capacity;
    }

    size_t get_connect_pool_capacity() const {
        return _conn_pool_max_size;
    }

private:
    // internal data struct
    struct EndPointWrapper {
        EndPoint end_point;
        std::unique_ptr<ConnectionPool> conn_pool;
        EndPointStatus status;
        int32_t err_count;

        explicit EndPointWrapper(const EndPoint& ep)
            : end_point(ep), conn_pool(), status(NORMAL), err_count(0) {}
        // allow move
        EndPointWrapper(EndPointWrapper&& other)
            : end_point(std::move(other.end_point)),
              conn_pool(std::move(other.conn_pool)),
              status(other.status),
              err_count(other.err_count) {}
        EndPointWrapper(const EndPointWrapper&) = delete;
        ~EndPointWrapper() {}

        void release_connect_pool() { conn_pool.reset(); }
        void create_connect_pool(size_t max_size);
        void set_death();
        void set_alive();
    };

private:
    ConnectionPtr fetch_connection_internal(const EndPoint& end_point, int32_t timeout_ms, bool *need_set_death);
    bool add_endpoint(const EndPoint& end_point);
    bool del_endpoint(const EndPoint& end_point);
    void set_endpoint_death(const EndPoint& end_point);
    void set_endpoint_alive(const EndPoint& end_point);

    bool check_error_count(int32_t error_cnt) const {
        return _max_error_count <= 0 || error_cnt < _max_error_count;
    }

private:
    // options
    ConnectionType _connection_type;
    int32_t _max_error_count;
    size_t _conn_pool_max_size;

    // all end points
    std::mutex _mutex;
    std::unordered_map<EndPoint, EndPointWrapper> _end_points;
    std::unordered_set<EndPoint> _dead_end_points;
};
 
} // end namespace wrpc
 
#endif // WRPC_NETWORK_END_POINT_MANAGER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

