/**
 * @file rr_load_balancer.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:37
 * @brief
 *  
 **/
 
#include "strategy/rr_load_balancer.h"

#include <algorithm>
#include <functional>

#include "utils/write_log.h"
 
namespace wrpc {
 
LoadBalancerOutput RRLoadBalancer::select(LoadBalancerContext& context) {
    if (context.retry_count == 0) {
        // set context
        context.data = (++_rr_index);
    }

    size_t offset = context.data + context.retry_count;
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_alive_list.empty()) {
        // alive list firstly
        size_t index = offset % _alive_list.size();
        DEBUG("LoadBalancer[%s] logid[%s] select in alive list",
                name().c_str(), context.logid.c_str());
        return std::make_pair(NET_SUCC, _alive_list[index]);
    } else if (!_dead_list.empty()) {
        // dead list secondly
        size_t index = offset % _dead_list.size();
        DEBUG("LoadBalancer[%s] logid[%s] select in dead list",
                name().c_str(), context.logid.c_str());
        return std::make_pair(NET_SUCC, _dead_list[index]);
    } else {
        DEBUG("LoadBalancer[%s] logid[%s] has no choosable end point",
                name().c_str(), context.logid.c_str());
        return std::make_pair(NET_NO_CHOOSABLE_END_POINT, EndPoint());
    }
}

static bool eq_equal(const EndPoint& ep1, const EndPoint& ep2) {
    return ep1 == ep2;
}
 
void RRLoadBalancer::on_set_death(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _dead_list.push_back(endpoint);
    remove_if(_alive_list.begin(), _alive_list.end(),
            std::bind(&eq_equal, endpoint, std::placeholders::_1));
    NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] death",
            name().c_str(), endpoint.to_string().c_str());
}

void RRLoadBalancer::on_set_alive(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _alive_list.push_back(endpoint);
    remove_if(_dead_list.begin(), _dead_list.end(),
            std::bind(&eq_equal, endpoint, std::placeholders::_1));
    NOTICE_LIGHT("LoadBalancer[%s] set end point [%s] alive",
            name().c_str(), endpoint.to_string().c_str());
}

void RRLoadBalancer::on_remove_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    remove_if(_alive_list.begin(), _alive_list.end(),
            std::bind(&eq_equal, endpoint, std::placeholders::_1));
    remove_if(_dead_list.begin(), _dead_list.end(),
            std::bind(&eq_equal, endpoint, std::placeholders::_1));
    NOTICE_LIGHT("LoadBalancer[%s] remove end point [%s]",
            name().c_str(), endpoint.to_string().c_str());
}

void RRLoadBalancer::on_add_one(const EndPoint& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _alive_list.push_back(endpoint);
    NOTICE_LIGHT("LoadBalancer[%s] add end point [%s]",
            name().c_str(), endpoint.to_string().c_str());
}

void RRLoadBalancer::on_update_all(const EndPointList& endpoint_list) {
    std::lock_guard<std::mutex> lock(_mutex);
    _alive_list.clear();
    _alive_list.reserve(endpoint_list.size());
    _alive_list.insert(_alive_list.end(), endpoint_list.begin(), endpoint_list.end());
    _dead_list.clear();
    //for (const EndPoint& ep : _alive_list) {
    //    DEBUG("LoadBalancer[%s] update end points[%s]", name().c_str(), ep.to_string().c_str());
    //}
    NOTICE_LIGHT("LoadBalancer[%s] update end points size[%lu]",
            name().c_str(), _alive_list.size());
}

REGISTER_LOAD_BALANCER(RRLoadBalancer, rr);

} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

