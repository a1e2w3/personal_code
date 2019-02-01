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

std::string EndPoint::to_string() const {
    static const size_t PORT_STRING_LEN = 5; // not contain ':', max port 65535
    char buffer[INET_ADDRSTRLEN + PORT_STRING_LEN + 1];

    if (inet_ntop(AF_INET, &_ip, buffer, INET_ADDRSTRLEN) == nullptr) {
        return "";
    }
    char* buf = buffer + strnlen(buffer, INET_ADDRSTRLEN);
    *buf = ':';
    ++buf;
    snprintf(buf, PORT_STRING_LEN + 1, "%u", _port);
    return std::string(buffer);
}


int EndPoint::to_sock_addr(struct sockaddr_in* addr) const {
    if (addr == nullptr || !is_valid()) {
        return NET_INVALID_ARGUMENT;
    }

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(_port);
    addr->sin_addr = _ip;
    return NET_SUCC;
}

size_t EndPoint::hash_code() const {
    return std::hash<EndPoint>()(*this);
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

