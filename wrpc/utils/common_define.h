/**
 * @file common_define.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 13:34:32
 * @brief 
 *  
 **/
 
#ifndef WRPC_UTILS_COMMON_DEFINE_H_
#define WRPC_UTILS_COMMON_DEFINE_H_
 
#include <stdint.h>
#include <string>
#include <string.h>

namespace wrpc {

typedef uint16_t port_t;
typedef uint32_t ControllerId;
 
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
 
#endif // WRPC_UTILS_COMMON_DEFINE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

