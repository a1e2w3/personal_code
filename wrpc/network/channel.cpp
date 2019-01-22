/**
 * @file channel.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:55:37
 * @brief 
 *  
 **/
 
#include "network/channel.h"

#include <mutex>
#include "instance_pool.h"

#include "network/connection.h"
#include "network/controller.h"

#include "utils/string_utils.h"
#include "utils/write_log.h"
 
namespace wrpc {
 
Channel::Channel()
    : _refresh_task(PeriodicTaskController::make_controller()),
      _health_check_task(PeriodicTaskController::make_controller()) {
    DEBUG("construct channel");
}
 
Channel::~Channel() {
    DEBUG("deconstruct channel");
    release();
}

ChannelPtr Channel::make_channel() {
    return ChannelPtr(new (std::nothrow) Channel(), [] (Channel* channel) {
        if (channel != nullptr) {
            delete channel;
        }
    });
}
 
int Channel::init(const std::string& address, const ChannelOptions& options) {
    // check protocol
    if (!RequestFactory::get_instance().is_registered(options.protocol)) {
        WARNING("Channel::init request protocol [%s] is not registereds",
                options.protocol.c_str());
        return NET_INVALID_ARGUMENT;
    }
    if (!ResponseFactory::get_instance().is_registered(options.protocol)) {
        WARNING("Channel::init response protocol [%s] is not registereds",
                options.protocol.c_str());
        return NET_INVALID_ARGUMENT;
    }

    std::string naming_protocol;
    if (0 != split_2_kv(address, "://", &naming_protocol, &_real_address)) {
        WARNING("Channel::init invalid address format: [%s]",
                address.c_str());
        return NET_INVALID_ARGUMENT;
    }

    _options = options;
    _naming_service = NamingServiceFactory::get_instance().make_unique(
            naming_protocol, naming_protocol);
    if (!_naming_service) {
        release();
        WARNING("Channel::init create naming service for address protocol [%s] failed, address: [%s]",
                naming_protocol.c_str(), address.c_str());
        return NET_INVALID_ARGUMENT;
    }

    _lb = LBFactory::get_instance().make_unique(
            _options.load_balancer, _options.load_balancer);
    if (!_lb) {
        release();
        WARNING("Channel::init load balancer for [%s] failed",
                _options.load_balancer.c_str());
        return NET_INVALID_ARGUMENT;
    }

    if (!_options.retry_policy.empty() && _options.retry_policy != _options.load_balancer) {
        _retry_policy = LBFactory::get_instance().make_unique(
                _options.retry_policy, _options.retry_policy);
        if (!_retry_policy) {
            release();
            WARNING("Channel::init retry policy for [%s] failed",
                    _options.retry_policy.c_str());
            return NET_INVALID_ARGUMENT;

        }
    }

    _endpoint_manager.reset(new (std::nothrow) EndPointManager());
    if (!_endpoint_manager) {
        release();
        WARNING("Channel::init allocate EndPointManager failed");
        return NET_INTERNAL_ERROR;
    }

    _endpoint_manager->set_connection_type(_options.connection_type);
    _endpoint_manager->set_max_error_count(_options.max_error_count_per_endpoint);
    _endpoint_manager->set_connect_pool_capacity(_options.max_connection_per_endpoint);

    // add listening
    _naming_service->add_observer(_endpoint_manager.get());
    _endpoint_manager->add_watcher(_lb.get());
    if (_retry_policy) {
        _endpoint_manager->add_watcher(_retry_policy.get());
    }

    // register back ground tasks
    _naming_service->refresh(_real_address);
    _refresh_task->start(std::bind(&Channel::refresh_endpoints, shared_from_this()),
            _options.update_end_points_interval);

    _health_check_task->start(
            std::bind(&Channel::do_health_check, shared_from_this()),
            _options.health_check_interval);
    return NET_SUCC;
}

int Channel::init(const EndPoint& end_point, const ChannelOptions& options) {
    EndPointList ep_list;
    ep_list.insert(end_point);
    return init(ep_list, options);
}

int Channel::init(const EndPointList& end_point_list, const ChannelOptions& options) {
    if (end_point_list.empty()) {
        WARNING("Channel::init empty end point list");
        return NET_INVALID_ARGUMENT;
    }

    // check protocol
    if (!RequestFactory::get_instance().is_registered(options.protocol)) {
        WARNING("Channel::init request protocol [%s] is not registereds",
                options.protocol.c_str());
        return NET_INVALID_ARGUMENT;
    }
    if (!ResponseFactory::get_instance().is_registered(options.protocol)) {
        WARNING("Channel::init response protocol [%s] is not registereds",
                options.protocol.c_str());
        return NET_INVALID_ARGUMENT;
    }

    _options = options;
    _lb = LBFactory::get_instance().make_unique(
            _options.load_balancer, _options.load_balancer);
    if (!_lb) {
        release();
        WARNING("Channel::init load balancer for [%s] failed",
                _options.load_balancer.c_str());
        return NET_INVALID_ARGUMENT;
    }

    if (!_options.retry_policy.empty() && _options.retry_policy != _options.load_balancer) {
        _retry_policy = LBFactory::get_instance().make_unique(
                _options.retry_policy, _options.retry_policy);
        if (!_retry_policy) {
            release();
            WARNING("Channel::init retry policy for [%s] failed",
                    _options.retry_policy.c_str());
            return NET_INVALID_ARGUMENT;

        }
    }

    _endpoint_manager.reset(new (std::nothrow) EndPointManager());
    if (!_endpoint_manager) {
        release();
        WARNING("Channel::init allocate EndPointManager failed");
        return NET_INTERNAL_ERROR;
    }
    _endpoint_manager->set_connection_type(_options.connection_type);
    _endpoint_manager->set_max_error_count(_options.max_error_count_per_endpoint);
    _endpoint_manager->set_connect_pool_capacity(_options.max_connection_per_endpoint);

    // update endpoint list
    _endpoint_manager->on_update(end_point_list);
    _lb->on_update_all(end_point_list);
    if (_retry_policy) {
        _retry_policy->on_update_all(end_point_list);
    }
    return NET_SUCC;
}

void Channel::release() {
    _refresh_task->cancel();
    _health_check_task->cancel();

    _endpoint_manager.reset();
    _retry_policy.reset();
    _lb.reset();
    _naming_service.reset();
}

IRequest* Channel::make_request() {
    return RequestFactory::get_instance().new_instance(_options.protocol);
}

IResponse* Channel::make_response() {
    return ResponseFactory::get_instance().new_instance(_options.protocol);
}

void Channel::destroy_request(IRequest* request) {
    RequestFactory::get_instance().destroy_instance(_options.protocol, request);
}

void Channel::destroy_response(IResponse* response) {
    ResponseFactory::get_instance().destroy_instance(_options.protocol, response);
}

#if WRPC_USE_CONTROLLER_INST_POOL_CAPACITY > 0
// 使用对象池
ControllerPtr Channel::create_controller() {
    typedef common::InstancePool<Controller, ChannelPtr&&, const RPCOptions&> ControllerInstPool;
    static ControllerInstPool* s_controller_pool = nullptr;
    static std::once_flag s_init_controller_pool_once;
    std::call_once(s_init_controller_pool_once, [&s_controller_pool] () -> void {
        s_controller_pool = new ControllerInstPool(
                WRPC_USE_CONTROLLER_INST_POOL_CAPACITY,
                nullptr,
                nullptr,
                [] (Controller* pointor, ChannelPtr&& channel, const RPCOptions& options) {
                    if (pointor != nullptr) {
                        return new (pointor) Controller(std::move(channel), options);
                    } else {
                        return pointor;
                    }
                },
                [] (Controller* pointor) {
                    if (pointor != nullptr) {
                        pointor->~Controller();
                    }
                });
    });
    return s_controller_pool->fetch_shared(shared_from_this(), _options.default_rpc_options());
}
#else
// 不使用对象池
ControllerPtr Channel::create_controller() {
    return ControllerPtr(new (std::nothrow) Controller(
            shared_from_this(), _options.default_rpc_options()), [] (Controller* controller) {
        if (controller != nullptr) {
            delete controller;
        }
    });
}
#endif

int Channel::fetch_connection(LoadBalancerContext& context,
        int32_t timeout_ms, ConnectionPtr& connection) {
    if (_lb && _endpoint_manager) {
        LoadBalancer* lb = nullptr;
        if (context.retry_count > 0 && _retry_policy) {
            // do retry with retry polict
            lb = _retry_policy.get();
        } else {
            lb = _lb.get();
        }
        LoadBalancerOutput lb_out = lb->select(context);
        if (lb_out.first == NET_SUCC) {
            connection = _endpoint_manager->fetch_connection(lb_out.second, timeout_ms);
            context.try_list.emplace_back(std::move(lb_out.second));
            if (!connection) {
                return NET_CONNECT_FAIL;
            }
        }
        return lb_out.first;
    } else {
        return NET_INTERNAL_ERROR;
    }
}

void Channel::giveback_connection(ConnectionPtr&& connection, bool close) {
    if (connection) {
        if (_endpoint_manager) {
            _endpoint_manager->giveback_connection(
                    connection->end_point(), std::move(connection), close);
        } else {
            connection.reset();
        }
    }
}
void Channel::feedback(const FeedbackInfo& info) {
    DEBUG("feedback ep[%s] code[%d] cost[%u] conn[%u] write[%u] read[%u]",
            info.endpoint.to_string().c_str(), info.code, info.total_cost,
            info.connect_cost, info.write_cost, info.read_cost);
    if (_lb) {
        _lb->feedback(info);
    }
}

void Channel::refresh_endpoints(ChannelWeakPtr channel) {
    ChannelPtr locked = channel.lock();
    if (locked && locked->_naming_service) {
        int ret = locked->_naming_service->refresh(locked->_real_address);
        if (ret == 0) {
            DEBUG("refresh endpoints for protocol [%s] address [%s] success.",
                locked->_naming_service->protocol().c_str(),
                locked->_real_address.c_str());
        } else {
            WARNING("refresh endpoints for protocol [%s] address [%s] failed with ret:%d.",
                    locked->_naming_service->protocol().c_str(),
                    locked->_real_address.c_str(),
                    ret);
        }
    }
}

void Channel::do_health_check(ChannelWeakPtr channel) {
    ChannelPtr locked = channel.lock();
    if (locked && locked->_endpoint_manager) {
        locked->_endpoint_manager->health_check(locked->_options.connect_timeout_ms);
        DEBUG("do health chack.");
    }
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

