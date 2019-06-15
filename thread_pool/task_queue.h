/**
 * @file task_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 17:11:12
 * @brief 任务队列定义
 *
 **/

#pragma once
 
#include "thread_pool/task.h"
 
namespace common {

typedef std::pair<TaskFunc, TaskAttr> TaskInfo;

class TaskQueue {
public:
    virtual ~TaskQueue() {}

    TaskId push_task(const TaskFunc& task_func) {
        return push_task(task_func, TaskAttr());
    }

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr) = 0;

    virtual TaskInfo pop_task() = 0;

    virtual bool cancel_task(TaskId task_id) = 0;

    virtual size_t queue_len() const = 0;

protected:
    static void do_nothing() {}
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

