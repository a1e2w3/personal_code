/**
 * @file hash_mod_load_balancer.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-13 13:36:26
 * @brief 
 *  
 **/
 
#include "strategy/hash_mod_load_balancer.h"
 
#include <algorithm>
#include <functional>

#include "utils/write_log.h"
 
namespace wrpc {
 
LoadBalancerOutput HashModLoadBalancer::select(LoadBalancerContext& context) {
    size_t offset = context.hash_code + context.retry_count;
    std::lock_guard<std::mutex> lock(_mutex);
    if (_candicates.empty()) {
        DEBUG("LoadBalancer[%s] logid[%s] has no choosable end point",
                name().c_str(), context.logid.c_str());
        return std::make_pair(NET_NO_CHOOSABLE_END_POINT, EndPoint());
    }

    const EndPoint& end_point = _candicates[offset % _candicates.size()];
    // 三种情况直接返回:
    // 1. 对于第一次尝试的, 不care是否为dead实例, 尽量保证hash效果
    // 2. _death.size() == _candicates.size(), 全部实例都是dead
    // 3. 选中的实例不是dead实例
    if (context.retry_count == 0 ||
        _death.size() == _candicates.size() ||
        _death.find(end_point) == _death.end()) {
        DEBUG("LoadBalancer[%s] logid[%s] select [%s]",
                name().c_str(), context.logid.c_str(), end_point.to_string().c_str());
        return std::make_pair(NET_SUCC, end_point);
    }

    // 尝试为重试的请求, 找一个alive的实例, 一定可以找到
    for (size_t i = 0; i < _candicates.size(); ++i) {
        const EndPoint& candicate = _candicates[(offset + i) % _candicates.size()];
        if (_death.find(candicate) == _death.end()) {
            DEBUG("LoadBalancer[%s] logid[%s] select [%s]",
                    name().c_str(), context.logid.c_str(), end_point.to_string().c_str());
            return std::make_pair(NET_SUCC, end_point);
        }
    }

    // should not happen
    return std::make_pair(NET_NO_CHOOSABLE_END_POINT, EndPoint());
}

static bool eq_equal(const EndPoint& ep1, const EndPoint& ep2) {
    return ep1 == ep2;
}
 
void HashModLoadBalancer::on_set_death(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _death.insert(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] death",
            name().c_str(), endpoint.to_string().c_str());
}

void HashModLoadBalancer::on_set_alive(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _death.erase(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] alive",
            name().c_str(), endpoint.to_string().c_str());
}

void HashModLoadBalancer::on_remove_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    remove_if(_candicates.begin(), _candicates.end(),
            std::bind(&eq_equal, endpoint, std::placeholders::_1));
    _death.erase(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] remove end point [%s]",
            name().c_str(), endpoint.to_string().c_str());
}

void HashModLoadBalancer::on_add_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _candicates.push_back(endpoint);
    _death.erase(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] add end point [%s]",
            name().c_str(), endpoint.to_string().c_str());
}

void HashModLoadBalancer::on_update_all(const EndPointList& endpoint_list) {
    std::lock_guard<std::mutex> lock(_mutex);
    _candicates.clear();
    _candicates.reserve(endpoint_list.size());
    _candicates.insert(_candicates.end(), endpoint_list.begin(), endpoint_list.end());
    _death.clear();
    //for (const EndPoint& ep : _candicates) {
    //    DEBUG("LoadBalancer[%s] update end points[%s]", name().c_str(), ep.to_string().c_str());
    //}
    NOTICE_LIGHT("LoadBalancer[%s] update end points size[%lu]",
            name().c_str(), _candicates.size());
}

REGISTER_LOAD_BALANCER(HashModLoadBalancer, hash_mod);

} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

