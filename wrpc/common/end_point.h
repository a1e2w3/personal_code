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

#include "common/ip_address.h"
#include "utils/common_define.h"
#include "utils/net_utils.h"

namespace wrpc {

class EndPoint;
typedef std::unordered_set<EndPoint> EndPointList;
 
// cannot change once created
class EndPoint {
private:
    IPAddress _ip;   // 网络字节序的ip
    port_t _port;    // 主机字节序的port

public:
    EndPoint() : _ip(), _port(0) {}
    EndPoint(const IPv4Address& ip, port_t port) : _ip(ip), _port(port) {}
    EndPoint(const IPv6Address& ip, port_t port) : _ip(ip), _port(port) {}
    EndPoint(const IPAddress& ip, port_t port) : _ip(ip), _port(port) {}
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

    int to_sock_addr(SocketAddress* addr) const;

    bool is_port_valid() const { return is_valid_port(_port); }
    bool is_valid() const { return is_port_valid(); }

    const IPAddress& ip() const { return _ip; }
    port_t port() const { return _port; }
    int domain() const { return _ip.domain; }

    std::string to_string() const;
    std::string get_ip_string() const { return ip_to_string(_ip); }

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
        size_t hash = std::hash<::wrpc::IPAddress>()(ep.ip());
        hash ^= (std::hash<::wrpc::port_t>()(ep.port()) << 1);
        return hash;
    }
};
} // end namespace std
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

