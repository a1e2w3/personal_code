/**
 * @file list_naming_service.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:57
 * @brief 
 *
 **/
 
#include "strategy/list_naming_service.h"

#include "utils/net_utils.h"
#include "utils/string_utils.h"
#include "utils/write_log.h"

namespace wrpc {

int ListNamingService::refresh(const std::string& address) {
    if (_last_address == address) {
        // no change
        return NET_SUCC;
    }

    EndPointList ep_list;
    std::vector<std::string> splits;
    string_split(address, ',', &splits);

    for (const std::string& split : splits) {
        std::string host;
        std::string port_str;

        if (0 == split_2_kv(split, ':', &host, &port_str)) {
            // parse port
            unsigned long port = 0;
            try {
                port = std::stoul(port_str);
            } catch(...) {
                WARNING("invalid port format: [%s]", split.c_str());
                continue;
            }

            if (!is_valid_port(port)) {
                WARNING("invalid port format: [%s]", split.c_str());
                continue;
            }

            // get ip from host
            IPv4Address host_ip = hostname_to_ip(host);
            if (!host_ip.empty()) {
                ep_list.emplace(host_ip, port);
            }
        }
    }
    notify_update(ep_list);
    _last_address = address;
    return NET_SUCC;
}

REGISTER_NAMING_SERVICE(ListNamingService, list)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

