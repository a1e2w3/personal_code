/**
 * @file file_naming_service.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:57
 * @brief 
 *  
 **/
 
#include "strategy/file_naming_service.h"

#include <fstream>

#include "utils/net_utils.h"
#include "utils/string_utils.h"
#include "utils/write_log.h"

namespace wrpc {

int FileNamingService::refresh(const std::string& address) {
    std::ifstream fin(address.c_str());
    if (!fin) {
        WARNING("FileNamingService open file [%s] failed", address.c_str());
        return -1;
    }

    EndPointList ep_list;
    std::string line;
    while (std::getline(fin, line)) {
        std::string host;
        std::string port_str;

        if (0 == split_2_kv(line, ':', &host, &port_str)) {
            // parse port
            port_t port = 0;
            try {
                port = std::stoul(port_str);
            } catch(...) {
                WARNING("invalid port format: [%s]", line.c_str());
                continue;
            }

            if (!is_valid_port(port)) {
                WARNING("invalid port format: [%s]", line.c_str());
                continue;
            }

            // get ip from host
            std::string host_ip = hostname_to_ip(host);
            if (!host_ip.empty()) {
                ep_list.emplace(host_ip, port);
            }
        }
    }
    notify_update(ep_list);
    return NET_SUCC;
}

REGISTER_NAMING_SERVICE(FileNamingService, file)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

