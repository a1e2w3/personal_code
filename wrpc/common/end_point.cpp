/**
 * @file end_point.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:58:36
 * @brief 
 *  
 **/
 
#include "common/end_point.h"
 
#include <memory>

namespace wrpc {

std::string ip_to_string(const IPAddress& ip) {
    char buffer[INET6_ADDRSTRLEN];
    if (inet_ntop(ip.domain, &ip.addr, buffer, INET6_ADDRSTRLEN) == nullptr) {
        return "";
    }
    return std::string(buffer);
}

IPAddress string_to_ip(const std::string& ip_str, int domain) {
    IPAddress ip(domain);
    int ret = inet_pton(domain, ip_str.c_str(), &ip.addr);
    return ip;
}

std::string EndPoint::to_string() const {
    static const size_t PORT_STRING_LEN = 5; // not contain ':', max port 65535
    char buffer[INET6_ADDRSTRLEN + PORT_STRING_LEN + 1];

    if (inet_ntop(_ip.domain, &_ip.addr, buffer, INET6_ADDRSTRLEN) == nullptr) {
        return "";
    }
    char* buf = buffer + strnlen(buffer, INET6_ADDRSTRLEN);
    *buf = ':';
    ++buf;
    snprintf(buf, PORT_STRING_LEN + 1, "%u", _port);
    return std::string(buffer);
}


int EndPoint::to_sock_addr(SocketAddress* addr) const {
    if (addr == nullptr || !is_valid()) {
        return NET_INVALID_ARGUMENT;
    }

    memset(addr, 0, sizeof(SocketAddress));
    if (_ip.domain == AF_INET) {
        // ipv4
        addr->addr_v4.sin_family = _ip.domain;
        addr->addr_v4.sin_port = htons(_port);
        addr->addr_v4.sin_addr = _ip.addr.ipv4;
    } else {
        // ipv6
        addr->addr_v6.sin6_family = _ip.domain;
        addr->addr_v6.sin6_port = htons(_port);
        addr->addr_v6.sin6_addr = _ip.addr.ipv6;
    }
    return NET_SUCC;
}

size_t EndPoint::hash_code() const {
    return std::hash<EndPoint>()(*this);
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

