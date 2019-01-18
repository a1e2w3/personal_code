/**
 * @file channel.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:55:33
 * @brief 一个下游服务的管理对象
 *        后端列表管理，刷新；负载均衡策略维护
 **/
 
#ifndef WRPC_NETWORK_CHANNEL_H_
#define WRPC_NETWORK_CHANNEL_H_

#include "common/options.h"
#include "common/rpc_feedback.h"
#include "interface/load_balancer.h"
#include "interface/message.h"
#include "interface/naming_service.h"
#include "network/network_common.h"
#include "network/end_point_manager.h"
#include "utils/background.h"
 
namespace wrpc {

class Channel : public std::enable_shared_from_this<Channel> {
private:
    Channel();
    ~Channel();
    // disallow copy
    Channel(const Channel&) = delete;
    Channel& operator = (const Channel&) = delete;

public:
    static ChannelPtr make_channel();

    int init(const std::string& address, const ChannelOptions& options);
    int init(const EndPoint& end_point, const ChannelOptions& options);
    int init(const EndPointList& end_point_list, const ChannelOptions& options);

    // should be called by ChannelPtr
    ControllerPtr create_controller();
    void destroy_controller(ControllerPtr&);
    void feedback(const FeedbackInfo& info);

private:
    friend class Controller;
    friend class RequestController;

    friend ChannelPtr make_channel();
    static void delete_channel(Channel*);
    static void delete_controller(Controller*);

    int fetch_connection(LoadBalancerContext& context, int32_t timeout_ms, ConnectionPtr& connection);
    void giveback_connection(ConnectionPtr&&, bool close);

    IRequest* make_request();
    IResponse* make_response();
    void destroy_request(IRequest*);
    void destroy_response(IResponse*);

    void release();

    // back ground task functions
    static void refresh_endpoints(ChannelWeakPtr);
    static void do_health_check(ChannelWeakPtr);

private:
    ChannelOptions _options;
    std::string _real_address;
    std::unique_ptr<EndPointManager> _endpoint_manager;
    NamingServicePtr _naming_service;
    LoadBalancerPtr _lb;
    LoadBalancerPtr _retry_policy;

    // back ground tasks
    PeriodicTaskControllerPtr _refresh_task;
    PeriodicTaskControllerPtr _health_check_task;
};
 
} // end namespace wrpc
 
#endif // WRPC_NETWORK_CHANNEL_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

