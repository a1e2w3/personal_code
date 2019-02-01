/**
 * @file common_define.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 13:34:32
 * @brief 
 *  
 **/
 
#ifndef WRPC_UTILS_COMMON_DEFINE_H_
#define WRPC_UTILS_COMMON_DEFINE_H_
 
#include <netinet/in.h> // struct in_addr
#include <stdint.h>
#include <string>
#include <string.h>

namespace wrpc {

typedef struct in_addr IPv4Address; // ipv4
typedef uint16_t port_t;

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
inline std::ostream& operator << (std::ostream& os, const IPv4Address& ip) {
    return os << ipv4_to_string(ip);
}
 
// network return codes

// error code need retry
#define NET_NEED_RETRY_MAX -1199
#define NET_EPOLL_FAIL -1107
#define NET_SEND_FAIL -1106
#define NET_RECV_FAIL -1105
#define NET_CONNECT_FAIL -1104
#define NET_TIMEOUT -1103
#define NET_INTERNAL_ERROR -1102
#define NET_UNKNOW_ERROR -1101
#define NET_NEED_RETRY_MIN -1100

// error code need not retry
#define NET_NOT_SUPPORTED -1008
#define NET_PARSE_MESSAGE_FAIL -1007
#define NET_MESSAGE_NOT_MATCH -1006
#define NET_NO_CHOOSABLE_END_POINT -1005
#define NET_BACKUP_SUCCESS -1004
#define NET_CANCELED -1003
#define NET_INVALID_ARGUMENT -1002
#define NET_DISCONNECTED -1001
#define NET_SUCC 0

typedef uint64_t ControllerId;
typedef uint64_t EventListenerData;

inline ControllerId getControllerIdFromListenerData(EventListenerData data) {
    return (data & 0xFFFFFFFF00000000UL) >> 32;
}

inline int getFdFromListenerData(EventListenerData data) {
    return static_cast<int>(data & 0x00000000FFFFFFFFUL);
}

inline EventListenerData buildListenerData(ControllerId cid, int fd) {
    return ((cid << 32) | static_cast<EventListenerData>(fd));
}

inline bool need_retry(int error_code) {
    return error_code >= NET_NEED_RETRY_MIN && error_code <= NET_NEED_RETRY_MAX;
}

// 析构然后原地重新构造，复用内存，节省一次内存分配和回收的开销
template<typename T, typename... Args>
inline T* reconstruct(T* pointer, Args&&... args) {
    if (pointer != nullptr) {
        pointer->~T();
        new (pointer) T(std::forward(args)...);
    }
    return pointer;
}

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
} // end namepace std
 
#endif // WRPC_UTILS_COMMON_DEFINE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

