/**
 * @file consistent_hash_load_balancer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-13 14:05:25
 * @brief 基于一致性hash的负载均衡
 *  
 **/
 
#ifndef WRPC_STRATEGY_CONSISTENT_HASH_LOAD_BALANCER_H_
#define WRPC_STRATEGY_CONSISTENT_HASH_LOAD_BALANCER_H_
 
#include <map>
#include <mutex>
#include <vector>
#include <unordered_set>

#include "interface/load_balancer.h"

// TODO 用虚拟节点解决节点数少的情况下, 负载不均的问题
namespace wrpc {

class ConsistentHashLoadBalancer : public LoadBalancer {
public:
    explicit ConsistentHashLoadBalancer(const std::string& name)
            : LoadBalancer(name), _node_count(0) {}
    virtual ~ConsistentHashLoadBalancer() {}

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

    size_t _node_count;
    // hash_code到节点的有序表
    std::map<size_t, EndPoint> _hash_ring;
    std::unordered_set<EndPoint> _death;
};

} // end namespace wrpc
 
#endif // WRPC_STRATEGY_CONSISTENT_HASH_LOAD_BALANCER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

