/**
 * @file fifo_task_queue.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-29 10:26:36
 * @brief 
 *  
 **/
 
#include "thread_pool/fifo_task_queue.h"

#include "thread_pool/task_queue_factory.h"
 
namespace common {
 
FifoTaskQueue::FifoTaskQueue(size_t pool_size) : _last_id(0), _pool(pool_size) {}
 
FifoTaskQueue::~FifoTaskQueue() {
	_task_ids.clear();
    for (TaskIdPair& pair : _queue) {
    	_pool.give_back(pair.second);
        pair.second = nullptr;
    }
}
 
TaskId FifoTaskQueue::push_task(const TaskFunc& task_func, const TaskAttr& attr) {
    TaskInfo *task = _pool.fetch(task_func, attr);
	std::lock_guard<std::mutex> lock(_mutex);
	TaskId id = (_last_id++);
	_queue.emplace_back(id, task);
	_task_ids.emplace(id, task);
	_cond.notify_one();
	return id;
}

TaskInfo FifoTaskQueue::pop_task() {
	std::unique_lock<std::mutex> lock(_mutex);
	bool found = false;
	TaskIdPair pair;
	while (!found) {
	    while (_queue.empty()) {
	        _cond.wait(lock);
	    }

	    // pop front
	    pair = _queue.front();

	    // find _task_ids
	    auto iter = _task_ids.find(pair.first);
	    found = (iter != _task_ids.end());
        if (found) {
            _task_ids.erase(iter);
        } else {
            _pool.give_back(pair.second);
        }
	    _queue.pop_front();
	}

    TaskInfo copy = *(pair.second);
    _pool.give_back(pair.second);
    return copy;
}

bool FifoTaskQueue::cancel_task(TaskId task_id) {
	std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _task_ids.find(task_id);
    if (iter == _task_ids.end()) {
        return false;
    } else {
        // release resource binding with task function
        iter->second->first = &TaskQueue::do_nothing;
        _task_ids.erase(iter);
        return true;
    }
}

REGISTER_QUEUE(fifo_queue, create_queue<FifoTaskQueue>);
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

