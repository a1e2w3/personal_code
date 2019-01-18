/**
 * @file rpc_feedback.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 16:47:49
 * @brief 
 *     每次rpc完成后, 反馈给LoadBalancer的feedback
 *  
 **/
 
#ifndef WRPC_COMMON_RPC_FEEDBACK_H_
#define WRPC_COMMON_RPC_FEEDBACK_H_
 
#include <stdint.h>
#include <string>

#include "common/end_point.h"
#include "utils/common_define.h"
 
namespace wrpc {
 
struct FeedbackInfo {
    EndPoint endpoint;
    int code;
    uint32_t connect_cost;
    uint32_t write_cost;
    uint32_t read_cost;
    uint32_t total_cost;

    FeedbackInfo()
        : code(NET_UNKNOW_ERROR),
          connect_cost(0),
          write_cost(0),
          read_cost(0),
          total_cost(0) {}

    void reset(const EndPoint& ep) {
        endpoint = ep;
        code = NET_UNKNOW_ERROR;
        connect_cost = 0;
        write_cost = 0;
        read_cost = 0;
        total_cost = 0;
    }
};
 
} // end namespace wrpc
 
#endif // WRPC_COMMON_RPC_FEEDBACK_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

