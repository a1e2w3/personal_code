/**
 * @file consistent_hash_load_balancer.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-13 14:05:30
 * @brief 
 *  
 **/
 
#include "strategy/consistent_hash_load_balancer.h"

#include <algorithm>
#include <functional>

#include "utils/write_log.h"
 
namespace wrpc {
 
LoadBalancerOutput ConsistentHashLoadBalancer::select(LoadBalancerContext& context) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_hash_ring.empty()) {
        DEBUG("LoadBalancer[%s] logid[%s] has no choosable end point",
                name().c_str(), context.logid.c_str());
        return std::make_pair(NET_NO_CHOOSABLE_END_POINT, EndPoint());
    }

    // 找到比hash code大的第一个节点
    auto iter = _hash_ring.upper_bound(context.hash_code);
    if (iter == _hash_ring.end()) {
        iter = _hash_ring.begin();
    }
    // 按重试次数退让
    for (uint32_t i = 0; i < context.retry_count; ++i) {
        ++iter;
        if (iter == _hash_ring.end()) {
            iter = _hash_ring.begin();
        }
    }

    // 三种情况直接返回:
    // 1. 对于第一次尝试的, 不care是否为dead实例, 尽量保证hash效果
    // 2. _node_count == _death.size(), 全部实例都是dead
    // 3. 选中的实例不是dead实例
    if (context.retry_count == 0 ||
            _node_count == _death.size() ||
            _death.find(iter->second) == _death.end()) {
        DEBUG("LoadBalancer[%s] logid[%s] select [%s]",
                name().c_str(), context.logid.c_str(), iter->second.to_string().c_str());
        return std::make_pair(NET_SUCC, iter->second);
    }

    // 尝试为重试的请求继续退让, 找一个alive的实例, 一定可以找到
    for (size_t i = 0; i < _hash_ring.size(); ++i) {
        ++iter;
        if (iter == _hash_ring.end()) {
            iter = _hash_ring.begin();
        }

        if (_death.find(iter->second) == _death.end()) {
            DEBUG("LoadBalancer[%s] logid[%s] select [%s]",
                    name().c_str(), context.logid.c_str(), iter->second.to_string().c_str());
            return std::make_pair(NET_SUCC, iter->second);
        }
    }

    // should not happen
    return std::make_pair(NET_NO_CHOOSABLE_END_POINT, EndPoint());
}
 
//static bool eq_equal(const EndPoint& ep1, const EndPoint& ep2) {
//    return ep1 == ep2;
//}
 
void ConsistentHashLoadBalancer::on_set_death(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_hash_ring.find(endpoint.hash_code()) != _hash_ring.end()) {
        _death.insert(endpoint);
        NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] death",
                name().c_str(), endpoint.to_string().c_str());
    }
}

void ConsistentHashLoadBalancer::on_set_alive(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _death.erase(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] alive",
            name().c_str(), endpoint.to_string().c_str());
}

void ConsistentHashLoadBalancer::on_remove_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _hash_ring.find(endpoint.hash_code());
    if (iter != _hash_ring.end()) {
        // 防止hash code冲突误删
        if (iter->second == endpoint) {
            _hash_ring.erase(iter);
            --_node_count;
            NOTICE_LIGHT("LoadBalancer[%s] remove end point [%s]",
                    name().c_str(), endpoint.to_string().c_str());
        }
        _death.erase(endpoint);
    }
}

void ConsistentHashLoadBalancer::on_add_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto ret = _hash_ring.emplace(endpoint.hash_code(), endpoint);
    if (ret.second) {
        ++_node_count;
        _death.erase(endpoint);
        NOTICE_LIGHT("LoadBalancer[%s] add end point [%s]",
                name().c_str(), endpoint.to_string().c_str());
    }
}

void ConsistentHashLoadBalancer::on_update_all(const EndPointList& endpoint_list) {
    std::lock_guard<std::mutex> lock(_mutex);
    _death.clear();
    for (const EndPoint& ep : endpoint_list) {
        auto ret = _hash_ring.emplace(ep.hash_code(), ep);
        if (ret.second) {
            ++_node_count;
        }
        //DEBUG("LoadBalancer[%s] update end points[%s]", name().c_str(), ep.to_string().c_str());
    }
    NOTICE_LIGHT("LoadBalancer[%s] update end points size[%lu]",
            name().c_str(), _node_count);
}

REGISTER_LOAD_BALANCER(ConsistentHashLoadBalancer, hash);
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

