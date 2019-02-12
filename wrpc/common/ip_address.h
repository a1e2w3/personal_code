/**
 * @file ip_address.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2019-02-12 16:51:12
 * @brief ip address definition
 *
 **/

#pragma once

#include <arpa/inet.h>
#include <memory.h>
#include <netinet/in.h> // struct in_addr
 
namespace wrpc {

typedef struct in_addr IPv4Address;  // ipv4

static const IPv4Address IPV4_ANY = { INADDR_ANY };    // 0.0.0.0
static const IPv4Address IPV4_NONE = { INADDR_NONE };  // 无效ip

// relational operators
// make IPv4Address can be the key of std::map and std::unordered_map
inline bool operator < (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr < rhs.s_addr;
}
inline bool operator > (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr > rhs.s_addr;
}
inline bool operator <= (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr <= rhs.s_addr;
}
inline bool operator >= (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr >= rhs.s_addr;
}
inline bool operator == (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr == rhs.s_addr;
}
inline bool operator != (const IPv4Address& lhs, const IPv4Address& rhs) {
    return lhs.s_addr != rhs.s_addr;
}

typedef struct in6_addr IPv6Address; // ipv6

// return 0 if equal, -1 if lhs < rhs, 1 if lhs > rhs
inline int ipv6_cmp(const IPv6Address& lhs, const IPv6Address& rhs) {
    for (size_t i = 0; i < sizeof(lhs.s6_addr) / sizeof(lhs.s6_addr[0]); ++i) {
        if (lhs.s6_addr[i] < rhs.s6_addr[i]) {
            return -1;
        } else if (lhs.s6_addr[i] > rhs.s6_addr[i]) {
            return 1;
        }
    }
    return 0;
}

// relational operators
// make IPv6Address can be the key of std::map and std::unordered_map
inline bool operator < (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) < 0;
}
inline bool operator > (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) > 0;
}
inline bool operator <= (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) <= 0;
}
inline bool operator >= (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) >= 0;
}
inline bool operator == (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) == 0;
}
inline bool operator != (const IPv6Address& lhs, const IPv6Address& rhs) {
    return ipv6_cmp(lhs, rhs) != 0;
}
 
struct IPAddress {
    int domain;
    union {
        IPv4Address ipv4;
        IPv6Address ipv6;
    } addr;

    explicit IPAddress(int domain_ = AF_INET) : domain(domain_) {
        memset(&addr, 0, sizeof(addr));
    }
    explicit IPAddress(IPv4Address ipv4) : domain(AF_INET) {
        addr.ipv4 = ipv4;
    }
    explicit IPAddress(IPv6Address ipv6) : domain(AF_INET6) {
        addr.ipv6 = ipv6;
    }
};

// return 0 if equal, -1 if lhs < rhs, 1 if lhs > rhs
inline int ip_cmp(const IPAddress& lhs, const IPAddress& rhs) {
    if (lhs.domain < rhs.domain) {
        return -1;
    } else if (lhs.domain > rhs.domain) {
        return 1;
    } else if (lhs.domain == AF_INET) {
        if (lhs.addr.ipv4 == rhs.addr.ipv4) {
            return 0;
        } else if (lhs.addr.ipv4 < rhs.addr.ipv4) {
            return -1;
        } else {
            return 1;
        }
    } else {
        return ipv6_cmp(lhs.addr.ipv6, rhs.addr.ipv6);
    }
}

// relational operators
// make IPv6Address can be the key of std::map and std::unordered_map
inline bool operator < (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) < 0;
}
inline bool operator > (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) > 0;
}
inline bool operator <= (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) <= 0;
}
inline bool operator >= (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) >= 0;
}
inline bool operator == (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) == 0;
}
inline bool operator != (const IPAddress& lhs, const IPAddress& rhs) {
    return ip_cmp(lhs, rhs) != 0;
}

inline std::ostream& operator << (std::ostream& os, const IPAddress& ip) {
    char buffer[INET6_ADDRSTRLEN];
    if (inet_ntop(ip.domain, &(ip.addr), buffer, INET6_ADDRSTRLEN) == nullptr) {
        return os;
    }
    return os << std::string(buffer);
}

/**
 * @brief ip转string
 *
 * @param [in] ip : const IPAddress &
 *
 * @return std::string
 *        ip address if success
 *        "" if failed
 */
std::string ip_to_string(const IPAddress& ip);

/**
 * @brief string转ip
 *
 * @param [in] ip : const std::string &
 * @param [in] domain : int
 *
 * @return IPAddress
 *        ip address if success
 *        INADDR_NONE if failed
 */
IPAddress string_to_ip(const std::string& ip_str, int domain = AF_INET);

/*
 * 兼容ipv4和ipv6版本的sock_addr
 *
 */
union SocketAddress {
    struct sockaddr_in addr_v4;
    struct sockaddr_in6 addr_v6;
};
 
} // end namespace wrpc

namespace std {
// specialization std::hash for wrpc::IPv4Address
// make IPv4Address can be the key of unordered_map
template<>
struct hash<::wrpc::IPv4Address> {
public:
    size_t operator () (const ::wrpc::IPv4Address& ip) const {
        return _hasher(ip.s_addr);
    }
private:
    std::hash<in_addr_t> _hasher;
};

// specialization std::hash for wrpc::IPv4Address
// make IPv4Address can be the key of unordered_map
template<>
struct hash<::wrpc::IPv6Address> {
public:
    size_t operator () (const ::wrpc::IPv6Address& ip) const {
        // 128位的addr拆分成两个uint64_t计算hash
        const uint64_t *p_i64 = reinterpret_cast<const uint64_t *>(ip.s6_addr);
        size_t hash = _hasher(p_i64[0]);
        hash ^= (_hasher(p_i64[1]) << 1);
        return hash;
    }
private:
    std::hash<uint64_t> _hasher;
};

// specialization std::hash for wrpc::IPAddress
// make IPv4Address can be the key of unordered_map
template<>
struct hash<::wrpc::IPAddress> {
public:
    size_t operator () (const ::wrpc::IPAddress& ip) const {
        if (ip.domain == AF_INET) {
            return hash<::wrpc::IPv4Address>()(ip.addr.ipv4);
        } else {
            return hash<::wrpc::IPv6Address>()(ip.addr.ipv6);
        }
    }
};

} // end namepace std
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

