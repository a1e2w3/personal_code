/**
 * @file hash_mod_load_balancer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-13 13:36:22
 * @brief 基于哈希取模的负载均衡, 按重试次数线性退让
 *        在节点变更不频繁(如list协议)的情况下, 达到按hash调度且效率较高
 *        节点变更频繁的情况下, 建议用一致性hash
 *  
 **/
 
#ifndef WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
#define WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
 
#include <mutex>
#include <vector>
#include <unordered_set>

#include "interface/load_balancer.h"

namespace wrpc {

class HashModLoadBalancer : public LoadBalancer {
public:
    explicit HashModLoadBalancer(const std::string& name) : LoadBalancer(name) {}
    virtual ~HashModLoadBalancer() {}

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

    std::vector<EndPoint> _candicates;
    std::unordered_set<EndPoint> _death;
};

} // end namespace wrpc
 
#endif // WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

