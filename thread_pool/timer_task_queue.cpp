/**
 * @file timer_task_queue.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-30 14:15:13
 * @brief
 *  
 **/
 
#include "thread_pool/timer_task_queue.h"
 
#include <iostream>
#include "thread_pool/task_queue_factory.h"
 
namespace common {
 
TimerTaskQueue::TimerTaskQueue(size_t pool_size) : _last_id(0), _pool(pool_size) {}

TimerTaskQueue::~TimerTaskQueue() {
    _task_id_map.clear();
    while (!_queue.empty()) {
        _pool.give_back(_queue.top().second);
        _queue.pop();
    }
}

TaskId TimerTaskQueue::push_task(const TaskFunc& task_func, const TaskAttr& attr) {
    TaskInfo* task = _pool.fetch(task_func, attr);
    std::lock_guard<std::mutex> lock(_mutex);
    TaskId t_id = (_last_id++);
    _task_id_map.emplace(t_id, task);
    _queue.emplace(t_id, task);
    _cond.notify_one();
    return t_id;
}

TaskInfo TimerTaskQueue::pop_task() {
    bool found = false;
    TaskIdPair top_elem;
	std::unique_lock<std::mutex> lock(_mutex);
	while (!found) {
        while (_queue.empty()) {
            _cond.wait(lock);
        }

        top_elem = _queue.top();
        auto iter = _task_id_map.find(top_elem.first);
        found = (iter != _task_id_map.end());
        if (!found) {
            _queue.pop();
            _pool.give_back(top_elem.second);
            continue;
        }

        // check exec time
        int64_t now = get_micro();
        int64_t exec_time = top_elem.second->second.exec_time;
        //fprintf(stdout, "top task exec time:%ld, now:%ld\n", exec_time, now);
        if (exec_time > now) {
            //fprintf(stdout, "wait:%ldus\n", (exec_time - now));
            found = false;
            _cond.wait_for(lock, Microseconds(exec_time - now));
            //fprintf(stdout, "wakeup\n");
            continue;
        } else {
            _task_id_map.erase(iter);
            _queue.pop();
        }
	}

	TaskInfo copy = *(top_elem.second);
	_pool.give_back(top_elem.second);
	return copy;
}

bool TimerTaskQueue::cancel_task(TaskId task_id) {
	std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _task_id_map.find(task_id);
    if (iter == _task_id_map.end()) {
        return false;
    } else {
        // release resource binding with functions
        iter->second->first = &TaskQueue::do_nothing;
        _task_id_map.erase(iter);
        return true;
    }
}

REGISTER_QUEUE(timer_queue, create_queue<TimerTaskQueue>);
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

