/**
 * @file common_flags.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-05 16:49:34
 * @brief 
 *  
 **/
 
#ifndef WRPC_UTILS_COMMON_FLAGS_H_
#define WRPC_UTILS_COMMON_FLAGS_H_
 
#include <stdint.h>
#include <string>

// use -DWRPC_XXX=xxx to modify these constants
// TODO use gflags

#ifndef WRPC_EVENT_DISPATCHER_NUMS
#define WRPC_EVENT_DISPATCHER_NUMS 1
#endif

#ifndef WRPC_EVENT_DISPATCHER_EVENT_SIZE
#define WRPC_EVENT_DISPATCHER_EVENT_SIZE 32
#endif

#ifndef WRPC_BACKGROUND_THREAD_NUMS
#define WRPC_BACKGROUND_THREAD_NUMS 1
#endif

#ifndef WRPC_CONNECT_TIMEOUT_FOR_HEALTH_CHECK
#define WRPC_CONNECT_TIMEOUT_FOR_HEALTH_CHECK 10 // 10ms
#endif

#ifndef WRPC_BNS_TIMEOUT
#define WRPC_BNS_TIMEOUT 1000 // 1000ms
#endif

#ifndef WRPC_REQ_MCPACK_BUFFER_SIZE
#define WRPC_REQ_MCPACK_BUFFER_SIZE (10 * 1024 * 1024) // 10MB
#endif

#ifndef WRPC_HTTP_MAX_HEADER_SIZE
#define WRPC_HTTP_MAX_HEADER_SIZE (80 * 1024) // 80KB
#endif

#ifndef WRPC_HTTP_MAX_HEADER_LINE_LEN
#define WRPC_HTTP_MAX_HEADER_LINE_LEN 1024 // 1KB
#endif
 
#endif // WRPC_UTILS_COMMON_FLAGS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

