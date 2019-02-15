/**
 * @file fifo_block_queue.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 14:25:50
 * @brief 
 *  
 **/
 
#include "fifo_block_queue.h"

#include <thread>

#include "task_queue_factory.h"
 
namespace common {
 
FifoBlockQueue::FifoBlockQueue(uint32_t capacity)
    : TaskQueue(),
      _queue(capacity),
      _pool(_queue.capacity()) {}

FifoBlockQueue::~FifoBlockQueue() {
    for (size_t i = 0; i < _queue.capacity(); ++i) {
        TaskInfo* t = _queue.set(nullptr, i);
        if (t != nullptr) {
            _pool.give_back(t);
        }
    }
}

TaskId FifoBlockQueue::push_task(const TaskFunc& task_func, const TaskAttr& attr) {
    TaskInfo* task = _pool.fetch(task_func, attr);
    return static_cast<TaskId>(_queue.push(task));
}

TaskInfo FifoBlockQueue::pop_task() {
    TaskInfo* task = _queue.pop();
    TaskInfo copy = *task;
    _pool.give_back(task);
    return copy;
}

bool FifoBlockQueue::cancel_task(TaskId task_id) {
    TaskInfo* ori_task = _queue.at(task_id);
    if (ori_task == nullptr) {
        return false;
    }

    TaskInfo* new_task = _pool.fetch(&TaskQueue::do_nothing, TaskAttr());
    while (!_queue.compare_exchange_weak(ori_task, new_task, task_id)) {
        ori_task = _queue.at(task_id);
        if (ori_task == nullptr) {
            _pool.give_back(new_task);
            new_task = nullptr;
            break;
        }
    }

    if (ori_task == nullptr) {
        return false;
    } else {
        _pool.give_back(ori_task);
        return true;
    }
}

REGISTER_QUEUE(fifo_block_queue_256, create_fifo_blocking_queue<256>);
REGISTER_QUEUE(fifo_block_queue_512, create_fifo_blocking_queue<512>);
REGISTER_QUEUE(fifo_block_queue_1024, create_fifo_blocking_queue<1024>);
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

