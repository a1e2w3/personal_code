/**
 * @file task.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 18:41:20
 * @brief 
 *  
 **/
 
#pragma once
 
#include <functional>

#include "thread_pool/timer.h"
 
namespace common { 

typedef int64_t TaskId;
const TaskId kInvalidId = -1;

typedef std::function<void()> TaskFunc;

struct TaskAttr {
    uint64_t priority;  // priority
    int64_t exec_time;  // expected exec time in us, <0 if whenever
    int64_t timeout;    // timeout in us

    TaskAttr() : priority(0), exec_time(get_micro()), timeout(0) {}
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

