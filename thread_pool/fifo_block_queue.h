/**
 * @file fifo_block_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 13:44:16
 * @brief ×èÈûÊ½fifo¶ÓÁÐ
 *
 **/
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "instance_pool.h"
#include "task_queue.h"
#include "timer.h"

namespace common {
 
class FifoBlockQueue : public TaskQueue {
public:
    explicit FifoBlockQueue(uint32_t capacity);
    virtual ~FifoBlockQueue();

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr);

    virtual TaskInfo pop_task();

    virtual bool cancel_task(TaskId task_id);

    virtual size_t queue_len() const {
        return _queue.queue_len();
    }

    uint32_t capacity() const {
        return _queue.capacity();
    }

private:
    // disallow copy
    FifoBlockQueue(const FifoBlockQueue&) = delete;
    FifoBlockQueue& operator = (const FifoBlockQueue&) = delete;

    typedef AtomicArrayQueue<TaskInfo> Queue;
    Queue _queue;

    // task info pool
    typedef InstancePool< TaskInfo, const TaskFunc&, const TaskAttr& > TaskInfoPool;
    TaskInfoPool _pool;
};

template<uint32_t capacity>
TaskQueue* create_fifo_blocking_queue() {
	return new FifoBlockQueue(capacity);
}
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

