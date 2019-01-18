/**
 * @file rr_load_balancer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:34
 * @brief 基于round robin的负载均衡
 *        按重试次数线性退让
 *  
 **/
 
#ifndef WRPC_STRATEGY_RR_LOAD_BALANCER_H_
#define WRPC_STRATEGY_RR_LOAD_BALANCER_H_
 
#include <mutex>
#include <vector>

#include "interface/load_balancer.h"
 
namespace wrpc {
 
class RRLoadBalancer : public LoadBalancer {
public:
    explicit RRLoadBalancer(const std::string& name) : LoadBalancer(name) {}
    virtual ~RRLoadBalancer() {}

public:
    virtual LoadBalancerOutput select(LoadBalancerContext& context);

    // on end point changed
    virtual void on_set_death(const EndPoint& endpoint);
    virtual void on_set_alive(const EndPoint& endpoint);
    virtual void on_remove_one(const EndPoint& endpoint);
    virtual void on_add_one(const EndPoint& endpoint);
    virtual void on_update_all(const EndPointList& endpoint_list);

private:
    std::mutex _mutex;
    volatile uint64_t _rr_index;

    std::vector<EndPoint> _alive_list;
    std::vector<EndPoint> _dead_list;
};
 
} // end namespace wrpc
 
#endif // WRPC_STRATEGY_RR_LOAD_BALANCER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

