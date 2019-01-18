/**
 * @file fifo_block_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 13:44:16
 * @brief 阻塞式fifo队列
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
    // capacity 需超过生产者和消费者数目的最大值
    // 实际capacity会被强制设成2的整数幂, 输入不能超过2^31
    explicit FifoBlockQueue(uint32_t capacity);
    virtual ~FifoBlockQueue();

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr);

    virtual TaskInfo pop_task();

    virtual bool cancel_task(TaskId task_id);

    virtual size_t queue_len() const {
        return _queue_len.load();
    }

    uint32_t capacity() const {
        return _capacity;
    }

private:
    // disallow copy
    FifoBlockQueue(const FifoBlockQueue&) = delete;
    FifoBlockQueue& operator = (const FifoBlockQueue&) = delete;

    TaskInfo wait_not_null_and_get(uint32_t index);
    void wait_null_and_set(TaskInfo* task, uint32_t index);

    // check interval if queue is full
    static const unsigned long kSleepWaitUs = 50;
    Microseconds _sleep_duration;

    const uint32_t _capacity;
    const uint32_t _index_mask;
    std::atomic<TaskInfo*>* _queue;
    std::atomic<uint32_t> _head;
    std::atomic<uint32_t> _tail;
    std::atomic<uint32_t> _queue_len;

    // lock for waiting
    std::mutex* _mutex_list;
    std::condition_variable* _cond_list;

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

