/**
 * @file timer_task_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-30 14:15:09
 * @brief 定时任务队列
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

class TimerTaskQueue : public TaskQueue {
public:
	explicit TimerTaskQueue(size_t pool_size = 128);
    virtual ~TimerTaskQueue();

    virtual TaskId push_task(const TaskFunc& task_func, const TaskAttr& attr);

    TaskId push_delay_task(uint64_t delay_us, const TaskFunc& task_func) {
        TaskAttr attr;
        attr.exec_time = get_micro() + delay_us;
        return push_task(task_func, attr);
    }

    virtual TaskInfo pop_task();

    virtual bool cancel_task(TaskId task_id);

    virtual size_t queue_len() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    // disallow copy
    TimerTaskQueue(const TimerTaskQueue&) = delete;
    TimerTaskQueue& operator = (const TimerTaskQueue&) = delete;

    typedef InstancePool< TaskInfo, const TaskFunc&, const TaskAttr& > TaskInfoPool;

    typedef std::pair<TaskId, TaskInfo*> TaskIdPair;
    struct TaskComparator {
        bool operator() (const TaskIdPair& t1, const TaskIdPair& t2) {
            return t1.second->second.exec_time > t2.second->second.exec_time;
        }
    };
    typedef std::priority_queue<TaskIdPair, std::deque<TaskIdPair>, TaskComparator> TimerQueue;
    TimerQueue _queue;
    std::unordered_map<TaskId, TaskInfo*> _task_id_map;
    mutable std::mutex _mutex;
    std::condition_variable _cond;
    TaskId _last_id;
    TaskInfoPool _pool;
};

} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

