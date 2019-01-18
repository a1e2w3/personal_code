/**
 * @file options.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 14:18:16
 * @brief 
 *  
 **/
 
#ifndef WRPC_COMMON_OPTIONS_H_
#define WRPC_COMMON_OPTIONS_H_
 
#include <stdint.h>
#include <string>
 
namespace wrpc {

enum ConnectionType {
    SHORT = 0,
    POOLED,
};

// options for rpc one times
struct RPCOptions {
    int32_t total_timeout_ms;
    int32_t connect_timeout_ms;
    int32_t backup_request_timeout_ms;
    uint32_t max_retry_num;

    RPCOptions()
        : total_timeout_ms(-1),
          connect_timeout_ms(-1),
          backup_request_timeout_ms(-1),
          max_retry_num(0) {}
};

struct ChannelOptions : public RPCOptions {
    std::string protocol;
    std::string load_balancer;
    std::string retry_policy;
    ConnectionType connection_type;
    size_t max_connection_per_endpoint; // used for connection pool
    int32_t  max_error_count_per_endpoint;

    uint32_t update_end_points_interval;
    uint32_t health_check_interval;

    ChannelOptions()
        : RPCOptions(),
          load_balancer("rr"),
          retry_policy(""),
          connection_type(SHORT),
          max_connection_per_endpoint(1),
          max_error_count_per_endpoint(-1),
          update_end_points_interval(5000),
          health_check_interval(1000) {}

    const RPCOptions& default_rpc_options() const { return *this; }
};
 
} // end namespace wrpc
 
#endif // WRPC_COMMON_OPTIONS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

