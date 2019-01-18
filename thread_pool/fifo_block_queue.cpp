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
 
static inline uint32_t highest_one_pos(uint32_t value) {
    uint32_t pos = 0;
    uint32_t val = value;
    if ((val & 0xffff0000) != 0) {
        pos += 16;
        val >>= 16;
    }

    if ((val & 0xff00) != 0) {
        pos += 8;
        val >>= 8;
    }

    if ((val & 0xf0) != 0) {
        pos += 4;
        val >>= 4;
    }

    if ((val & 0x0c) != 0) {
        pos += 2;
        val >>= 2;
    }

    if ((val & 0x02) != 0) {
        pos += 1;
        val >>= 1;
    }

    if (val != 0) {
        pos += 1;
    }
    return pos;
}

#define UPTO_POW_OF_2(val) ((val) == 0 ? 1 : 1 << (highest_one_pos((val) - 1)))

FifoBlockQueue::FifoBlockQueue(uint32_t capacity)
    : TaskQueue(),
      _sleep_duration(kSleepWaitUs),
      _capacity(UPTO_POW_OF_2(capacity)),
      _index_mask(_capacity - 1),
      _queue(new std::atomic<TaskInfo*>[_capacity]),
      _head(0),
      _tail(0),
      _queue_len(0),
      _mutex_list(new std::mutex[_capacity]),
      _cond_list(new std::condition_variable[_capacity]),
      _pool(_capacity) {
    for (uint32_t i = 0; i < _capacity; ++i) {
        _queue[i].store(nullptr);
    }
}

FifoBlockQueue::~FifoBlockQueue() {
    for (size_t i = 0; i < _capacity; ++i) {
        TaskInfo* task = _queue[i].exchange(nullptr);
        if (task != nullptr) {
            _pool.give_back(task);
        }
    }
    delete[] _queue;
    delete[] _mutex_list;
    delete[] _cond_list;
}

TaskId FifoBlockQueue::push_task(const TaskFunc& task_func, const TaskAttr& attr) {
    TaskInfo* task = _pool.fetch(task_func, attr);
    // 先占用槽位
    uint32_t index = _head++;
    // 等待Task被取走再设置
    wait_null_and_set(task, index);
    return static_cast<TaskId>(index);
}

TaskInfo FifoBlockQueue::pop_task() {
    // 先占用槽位
    uint32_t index = _tail++;
    // 等待有Task再返回
    return wait_not_null_and_get(index);
}

bool FifoBlockQueue::cancel_task(TaskId task_id) {
    uint32_t real_idx = task_id & _index_mask;
    TaskInfo* ori_task = _queue[real_idx].load();
    if (ori_task == nullptr) {
        return false;
    }

    TaskInfo* new_task = _pool.fetch(&TaskQueue::do_nothing, TaskAttr());
    while (!_queue[real_idx].compare_exchange_weak(ori_task, new_task)) {
        std::this_thread::sleep_for(_sleep_duration);
        ori_task = _queue[real_idx].load();
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

TaskInfo FifoBlockQueue::wait_not_null_and_get(uint32_t index) {
    uint32_t real_idx = index & _index_mask;
    std::unique_lock<std::mutex> lock(_mutex_list[real_idx]);
    // exchange _queue[real_idx] value with NULL atomicly
    TaskInfo* task = _queue[real_idx].exchange(nullptr);
    while (task == nullptr) {
        _cond_list[real_idx].wait(lock);
        task = _queue[real_idx].exchange(nullptr);
    }
    --_queue_len;
    TaskInfo copy = *task;
    _pool.give_back(task);
    return copy;
}

void FifoBlockQueue::wait_null_and_set(TaskInfo* task, uint32_t index) {
    uint32_t real_idx = index & _index_mask;
    TaskInfo* expected_addr = nullptr;
    // exchange _queue[real_idx] value with task
    // only if _queue[real_idx] value equals with expected_addr atomicly
    while (!_queue[real_idx].compare_exchange_weak(expected_addr, task)) {
        std::this_thread::sleep_for(_sleep_duration);
        expected_addr = nullptr;
    }
    ++_queue_len;
    // lock to avoid wait after notify
    std::lock_guard<std::mutex> lock(_mutex_list[real_idx]);
    _cond_list[real_idx].notify_one();
    return;
}

REGISTER_QUEUE(fifo_block_queue_256, create_fifo_blocking_queue<256>);
REGISTER_QUEUE(fifo_block_queue_512, create_fifo_blocking_queue<512>);
REGISTER_QUEUE(fifo_block_queue_1024, create_fifo_blocking_queue<1024>);
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

