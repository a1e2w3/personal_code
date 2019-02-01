/**
 * @file bns_naming_service.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 14:05:30
 * @brief 
 *  
 **/
 
#include "strategy/bns_naming_service.h"
 
#include <list>

#include "webfoot_naming.h"

#include "utils/common_flags.h"
#include "utils/net_utils.h"
#include "utils/write_log.h"
 
namespace wrpc {
 
int BNSNamingService::refresh(const std::string& address) {
    std::list<webfoot::InstanceInfo> host_list;

    int ret = webfoot::get_instance_by_service(address.c_str(), &host_list, WRPC_BNS_TIMEOUT);
    if (0 != ret) {
        WARNING("get_instance_by_service for bns[%s] failed, ret:[%d:%s]",
                address.c_str(), ret, webfoot::error_to_string(ret));
        return ret;
    }

    if (host_list.empty()) {
        WARNING("bns[%s] hostlist is empty", address.c_str());
        return -1;
    }

    EndPointList ep_list;
    for (const webfoot::InstanceInfo& info : host_list) {
        if (info.is_stale()) {
            DEBUG("filter stale instance[%s:%d] for bns[%s]",
                    info.host_ip_str().c_str(), info.port(), address.c_str());
            continue;
        }
        if (!is_valid_port(info.port())) {
            WARNING("filter invalie port instance[%s:%d] for bns[%s]",
                    info.host_ip_str().c_str(), info.port(), address.c_str());
            continue;
        }

        DEBUG("get instance[%s:%d] for bns[%s]",
                info.host_ip_str().c_str(), info.port(), address.c_str());
        ep_list.emplace(string_to_ipv4(info.host_ip_str()), info.port());
    }

    if (ep_list.empty()) {
        WARNING("bns[%s] got empty end point list", address.c_str());
        return -1;
    }

    notify_update(ep_list);
    return NET_SUCC;
}

REGISTER_NAMING_SERVICE(BNSNamingService, bns)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

