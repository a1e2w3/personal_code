/**
 * @file fifo_task_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-29 10:26:32
 * @brief 非阻塞式fifo队列
 *  
 **/
 
#pragma once
 
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>

#include "task_queue.h"
#include "instance_pool.h"
 
namespace common {
 
class FifoTaskQueue : public TaskQueue {
public:
    explicit FifoTaskQueue(size_t pool_size = 128);
    virtual ~FifoTaskQueue();

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr);

    virtual TaskInfo pop_task();

    virtual bool cancel_task(TaskId task_id);

    virtual size_t queue_len() const {
    	std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    mutable std::mutex _mutex;
    std::condition_variable _cond;

    typedef std::pair<TaskId, TaskInfo*> TaskIdPair;
    std::deque<TaskIdPair> _queue;
    std::unordered_map<TaskId, TaskInfo*> _task_ids;
    TaskId _last_id;

    typedef InstancePool< TaskInfo, const TaskFunc&, const TaskAttr& > TaskInfoPool;
    TaskInfoPool _pool;
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

