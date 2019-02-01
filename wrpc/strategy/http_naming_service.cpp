/**
 * @file http_naming_service.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 15:17:46
 * @brief 
 *  
 **/
 
#include "strategy/http_naming_service.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "utils/net_utils.h"
#include "utils/string_utils.h"
#include "utils/write_log.h"
 
namespace wrpc {
 
int HttpNamingService::refresh(const std::string& address) {
    unsigned long port = 80; // default port as 80
    std::string domain;
    std::string port_str;
    if (0 == split_2_kv(address, ':', &domain, &port_str)) {
        try {
            port = std::stoul(port_str);
        } catch(...) {
            WARNING("invalid port format: [%s]", address.c_str());
            return -1;
        }

        if (!is_valid_port(port)) {
            WARNING("invalid port format: [%s]", address.c_str());
            return -1;
        }
    } else {
        domain = address;
    }


    struct addrinfo hints;
    struct addrinfo *res = nullptr;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   // ipv4
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(domain.c_str(), protocol().c_str(), &hints, &res);
    if (ret != 0 || res == nullptr) {
        WARNING("getaddrinfo for domain[%s] protocol[%s] fail, errno:[%d:%s]",
                domain.c_str(), protocol().c_str(), ret, gai_strerror(ret));
        return -1;
    }

    EndPointList ep_list;
    for (auto cursor = res; cursor != nullptr; cursor = cursor->ai_next) {
        struct sockaddr_in * addr = reinterpret_cast<struct sockaddr_in *>(cursor->ai_addr);
        //port_t ep_port = addr->sin_port;
        port_t ep_port = port;
        ep_list.emplace(addr->sin_addr, ep_port);
        DEBUG("resolved instance [%s:%u] address [%s] protocol [%s].",
                ipv4_to_string(addr->sin_addr).c_str(), ep_port, address.c_str(), protocol().c_str());
    }
    freeaddrinfo(res);

    if (ep_list.empty()) {
        WARNING("address[%s] protocol[%s] got empty end point list",
                address.c_str(), protocol().c_str());
        return -1;
    }
    notify_update(ep_list);
    DEBUG("resolve address [%s] for protocol [%s] success.",
            address.c_str(), protocol().c_str());
    return NET_SUCC;
}
 
REGISTER_NAMING_SERVICE(HttpNamingService, http)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

