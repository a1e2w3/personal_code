/**
 * @file end_point.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:58:33
 * @brief 
 *  
 **/

#pragma once
 
#include <arpa/inet.h>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <string>
#include <unordered_set>

#include "utils/common_define.h"
#include "utils/net_utils.h"

namespace wrpc {

class EndPoint;
typedef std::unordered_set<EndPoint> EndPointList;
 
// cannot change once created
class EndPoint {
private:
    IPv4Address _ip;
    port_t _port;

public:
    EndPoint() : _ip(IPV4_ANY), _port(0){}
    EndPoint(const IPv4Address& ip, port_t port) : _ip(ip), _port(port) {}
    ~EndPoint() {}

    // allow copy
    EndPoint(const EndPoint& other) : _ip(other._ip), _port(other._port) {}
    EndPoint& operator = (const EndPoint& other) {
        if (&other != this) {
            _ip = other._ip;
            _port = other._port;
        }
        return *this;
    }

    int to_sock_addr(struct sockaddr_in* addr) const;

    bool is_port_valid() const { return is_valid_port(_port); }
    bool is_ip_valid() const { return _ip.s_addr != INADDR_NONE; }
    bool is_valid() const { return is_port_valid() && is_ip_valid(); }

    const IPv4Address& ip() const { return _ip; }
    port_t port() const { return _port; }

    std::string to_string() const;
    std::string get_ip_string() const { return ipv4_to_string(_ip); }

    size_t hash_code() const;
};

// relational operators
// make EndPoint can be the key of std::map and std::unordered_map
inline bool operator == (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.port() == ep2.port() && ep1.ip() == ep2.ip();
}
inline bool operator != (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.port() != ep2.port() || ep1.ip() != ep2.ip();
}
inline bool operator < (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.ip() < ep2.ip()) {
        return true;
    } else if (ep1.ip() == ep2.ip()) {
        return ep1.port() < ep2.port();
    } else {
        return false;
    }
}
inline bool operator <= (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.ip() < ep2.ip()) {
        return true;
    } else if (ep1.ip() == ep2.ip()) {
        return ep1.port() <= ep2.port();
    } else {
        return false;
    }
}
inline bool operator > (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.ip() > ep2.ip()) {
        return true;
    } else if (ep1.ip() == ep2.ip()) {
        return ep1.port() > ep2.port();
    } else {
        return false;
    }
}
inline bool operator >= (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.ip() > ep2.ip()) {
        return true;
    } else if (ep1.ip() == ep2.ip()) {
        return ep1.port() >= ep2.port();
    } else {
        return false;
    }
}
inline std::ostream& operator << (std::ostream& os, const EndPoint& ep) {
    return os << ep.to_string();
}

} // end namespace wrpc

namespace std {

// specialization std::hash for wrpc::EndPoint
// make EndPoint can be the key of unordered_map
template<>
struct hash<::wrpc::EndPoint> {
public:
    size_t operator () (const ::wrpc::EndPoint& ep) const {
        uint64_t value = static_cast<uint64_t>(ep.port());
        value &= (static_cast<uint64_t>(ep.ip().s_addr) << 16);
        return _hasher(value);
    }
private:
    std::hash<uint64_t> _hasher;
};
} // end namespace std
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

