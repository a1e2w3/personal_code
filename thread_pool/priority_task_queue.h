/**
 * @file priority_task_queue.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 16:29:01
 * @brief 优先级队列
 *
 **/

#pragma once
 
#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_map>
 
#include "instance_pool.h"
#include "task_queue.h"
#include "timer.h"
 
namespace common {

class PriorityTaskQueue : public TaskQueue {
public:
	explicit PriorityTaskQueue(size_t pool_size = 128);
    virtual ~PriorityTaskQueue();

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr);

    virtual TaskInfo pop_task();

    virtual bool cancel_task(TaskId task_id);

    virtual size_t queue_len() const {
    	std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    // disallow copy
    PriorityTaskQueue(const PriorityTaskQueue&) = delete;
    PriorityTaskQueue& operator = (const PriorityTaskQueue&) = delete;

    typedef std::pair<TaskId, TaskInfo*> TaskIdPair;
    struct TaskComparator {
        bool operator() (const TaskIdPair& t1, const TaskIdPair& t2) {
            return t1.second->second.priority > t2.second->second.priority;
        }
    };
    typedef std::priority_queue<TaskIdPair, std::deque<TaskIdPair>, TaskComparator> PriorityQueue;
    PriorityQueue _queue;
    std::unordered_map<TaskId, TaskInfo*> _task_id_map;
    mutable std::mutex _mutex;
    std::condition_variable _cond;
    TaskId _last_id;

    typedef InstancePool< TaskInfo, const TaskFunc&, const TaskAttr& > TaskInfoPool;
    TaskInfoPool _pool;
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

