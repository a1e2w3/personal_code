/**
 * @file atomic_array_queue.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2019-02-15 13:30:22
 * @brief 阻塞式fifo队列
 *        只负责对指针排队, 不负责队列内指针生命周期管理
 *        使用nullptr作为槽位为空的标记, 不允许插入nullptr
 *
 **/

#pragma once

#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include "timer.h"
 
namespace common {

inline uint32_t highest_one_pos(uint32_t value) {
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

template<typename T>
struct AtomicArrayQueueTrait {
    using element_type = typename std::remove_reference<T>::type;
    using pointer = element_type*;
};

template<>
struct AtomicArrayQueueTrait<void> {
    using pointer = void*;
};
 
template<typename T>
class AtomicArrayQueue {
public:
    using pointer = typename AtomicArrayQueueTrait<T>::pointer;
public:
    // capacity 需超过生产者和消费者数目的最大值
    // 实际capacity会被强制设成2的整数幂, 输入不能超过2^31
    explicit AtomicArrayQueue(uint32_t capacity)
        : _sleep_duration(kSleepWaitUs),
          _capacity(UPTO_POW_OF_2(capacity)),
          _index_mask(_capacity - 1),
          _queue(new std::atomic<pointer>[_capacity]),
          _head(0),
          _tail(0),
          _queue_len(0),
          _mutex_list(new std::mutex[_capacity]),
          _cond_list(new std::condition_variable[_capacity]) {
        for (uint32_t i = 0; i < _capacity; ++i) {
            _queue[i].store(nullptr);
        }
    }
    ~AtomicArrayQueue() {
        for (size_t i = 0; i < _capacity; ++i) {
            _queue[i].exchange(nullptr);
        }
        delete[] _queue;
        delete[] _mutex_list;
        delete[] _cond_list;
    }

    ssize_t push(pointer p) {
        if (p != nullptr) {
            // 先占用槽位
            uint32_t index = _head++;
            // 等待Task被取走再设置
            wait_null_and_set(p, index);
            return index;
        }
        return -1;
    }

    pointer pop() {
        // 先占用槽位
        uint32_t index = _tail++;
        // 等待有Item再返回
        return wait_not_null_and_get(index);
    }

    // 直接根据index取对应槽位的指针, 不会修改槽位
    pointer at(uint32_t index) const {
        return _queue[index & _index_mask].load();
    }
    // 直接根据index修改对应槽位的指针, 返回槽位修改之前的值
    pointer set(pointer p, uint32_t index) {
        return _queue[index & _index_mask].exchange(p);
    }
    // 直接根据index修改对应槽位的指针, 需满足槽位原有值等于expected
    bool compare_exchange_weak(pointer expected, pointer new_val, uint32_t index) {
        return _queue[index & _index_mask].compare_exchange_weak(expected, new_val);
    }

    size_t queue_len() const {
        return _queue_len.load();
    }

    uint32_t capacity() const {
        return _capacity;
    }

private:
    // disallow copy and move
    AtomicArrayQueue(const AtomicArrayQueue&) = delete;
    AtomicArrayQueue(AtomicArrayQueue&&) = delete;
    AtomicArrayQueue& operator = (const AtomicArrayQueue&) = delete;
    AtomicArrayQueue& operator = (AtomicArrayQueue&&) = delete;

    pointer wait_not_null_and_get(uint32_t index) {
        uint32_t real_idx = index & _index_mask;
        std::unique_lock<std::mutex> lock(_mutex_list[real_idx]);
        // exchange _queue[real_idx] value with NULL atomicly
        pointer p = _queue[real_idx].exchange(nullptr);
        while (p == nullptr) {
            _cond_list[real_idx].wait(lock);
            p = _queue[real_idx].exchange(nullptr);
        }
        --_queue_len;
        return p;
    }

    void wait_null_and_set(pointer p, uint32_t index) {
        assert(p != nullptr);
        uint32_t real_idx = index & _index_mask;
        pointer expected_addr = nullptr;
        // exchange _queue[real_idx] value with task
        // only if _queue[real_idx] value equals with expected_addr atomicly
        while (!_queue[real_idx].compare_exchange_weak(expected_addr, p)) {
            std::this_thread::sleep_for(_sleep_duration);
            expected_addr = nullptr;
        }
        ++_queue_len;
        // lock to avoid wait after notify
        std::lock_guard<std::mutex> lock(_mutex_list[real_idx]);
        _cond_list[real_idx].notify_one();
        return;
    }

private:
    // check interval if queue is full
    static const unsigned long kSleepWaitUs = 50;
    Microseconds _sleep_duration;

    const uint32_t _capacity;
    const uint32_t _index_mask;
    std::atomic<pointer>* _queue;
    std::atomic<uint32_t> _head;
    std::atomic<uint32_t> _tail;
    std::atomic<uint32_t> _queue_len;

    // lock for waiting
    std::mutex* _mutex_list;
    std::condition_variable* _cond_list;
};

} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

