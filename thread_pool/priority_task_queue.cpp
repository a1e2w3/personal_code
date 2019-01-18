/**
 * @file priority_task_queue.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 16:29:01
 * @brief 
 *  
 **/
 
#include "priority_task_queue.h"

#include "task_queue_factory.h"
 
namespace common {
 
PriorityTaskQueue::PriorityTaskQueue(size_t pool_size) : _last_id(0), _pool(pool_size) {}

PriorityTaskQueue::~PriorityTaskQueue() {
	_task_id_map.clear();
    while (!_queue.empty()) {
    	_pool.give_back(_queue.top().second);
        _queue.pop();
    }
}

TaskId PriorityTaskQueue::push_task(const TaskFunc& task_func, const TaskAttr& attr) {
    TaskInfo* task = _pool.fetch(task_func, attr);
	std::lock_guard<std::mutex> lock(_mutex);
	TaskId t_id = (_last_id++);
	_queue.emplace(t_id, task);
	_task_id_map.emplace(t_id, task);
	_cond.notify_one();
	return t_id;
}

TaskInfo PriorityTaskQueue::pop_task() {
    bool found = false;
    TaskIdPair top_elem;
	std::unique_lock<std::mutex> lock(_mutex);
	while (!found) {
        while (_queue.empty()) {
            _cond.wait(lock);
        }
	    
        top_elem = _queue.top();
        _queue.pop();
        auto iter = _task_id_map.find(top_elem.first);
        found = (iter != _task_id_map.end());
        if (!found) {
        	_pool.give_back(top_elem.second);
        } else {
        	_task_id_map.erase(iter);
        }
	}

	TaskInfo copy = *(top_elem.second);
	_pool.give_back(top_elem.second);
	return copy;
}

bool PriorityTaskQueue::cancel_task(TaskId task_id) {
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

REGISTER_QUEUE(priority_queue, create_queue<PriorityTaskQueue>);
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

