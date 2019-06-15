/**
 * @file thread_pool.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 17:27:55
 * @brief 
 *  
 **/
 
#include "thread_pool/thread_pool.h"

#include <algorithm>
#include <assert.h>
#include <chrono>
#include <deque>
#include <functional>
#include <iostream>

#include "thread_pool/timer.h"

namespace common {
 
ThreadPool::ThreadPool(uint32_t thread_num)
    : _thread_num(thread_num),
      _queue(nullptr),
      _threads(nullptr),
      _stop(false),
      _is_running(false),
      _ctrl_waiting(false) {}
 
ThreadPool::~ThreadPool() {
    stop(false);
}
 
bool ThreadPool::start(TaskQueue* queue) {
    if (!queue || _thread_num == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_ctrl_mutex);
    if (is_running()) {
        return false;
    }

    // set queue
    _queue = queue;

    // start threads
    _stop.store(false);
    _is_running.store(true);
    _threads.reset(new std::thread[_thread_num]);
    for (uint32_t i = 0; i < _thread_num; ++i) {
        std::thread t(std::bind(&ThreadPool::thread_run_wrapper, this, i));
        _threads[i].swap(t);
    }
    return true;
}

bool ThreadPool::stop(bool wait) {
    std::unique_lock<std::mutex> lock(_ctrl_mutex);
    if (!is_running()) {
        return false;
    }
    assert(_queue != nullptr);

    if (wait) {
        _ctrl_waiting.store(true);
        // wait until task queue empty
        while(_queue->queue_len() > 0) {
            _ctrl_cond.wait_for(lock, Microseconds(100));
        }
    }

    _ctrl_waiting.store(false);
    _stop.store(true);
    // push empty task to awake blocking threads
    for (size_t i = 0; i < _thread_num; ++i) {
        _queue->push_task([]() {});
    }

    // join
    for (size_t i = 0; i < _thread_num; ++i) {
        _threads[i].join();
    }

    _threads.reset();
    _is_running.store(false);
    _queue = nullptr;
    return true;
}

#ifdef PERF_TEST
static std::mutex g_perf_mutex;
static std::deque<int64_t> g_sched_delays;

void print_sched_pretiles() {
    std::lock_guard<std::mutex> perf_lock(g_perf_mutex);
    if (g_sched_delays.empty()) {
        return;
    }
    double delay_sum = 0.0d;
    for (int64_t delay : g_sched_delays) {
        delay_sum += delay;
    }
    // print pretile
    std::sort(g_sched_delays.begin(), g_sched_delays.end());
    size_t sample_cnt = g_sched_delays.size();
    std::cout << "Sched pretile: " << std::endl;
    std::cout << "cnt: " << g_sched_delays.size()
              << " avg: " << delay_sum / sample_cnt
              << " 50: " << g_sched_delays[sample_cnt * 0.5] 
              << " 80: " << g_sched_delays[sample_cnt * 0.8]
              << " 90: " << g_sched_delays[sample_cnt * 0.9]
              << " 95: " << g_sched_delays[sample_cnt * 0.95]
              << " 97: " << g_sched_delays[sample_cnt * 0.97]
              << " 99: " << g_sched_delays[sample_cnt * 0.99]
              << " 99.9: " << g_sched_delays[sample_cnt * 0.999]
              << " max: " << g_sched_delays.back() << std::endl;
    g_sched_delays.clear();
}
#else
inline void print_sched_pretiles() {}
#endif

void ThreadPool::thread_run_wrapper(size_t /*thread_index*/) {
    assert(_queue != nullptr);

#ifdef PERF_TEST
    std::deque<int64_t> sched_delays;
#endif
    while(!_stop.load()) {
        TaskInfo task = _queue->pop_task();
        MicrosecondsTimer timer;
        _counter.schedule_delay += (timer.start_time() - task.second.exec_time);
#ifdef PERF_TEST
        sched_delays.push_back(timer.start_time() - task.second.exec_time);
#endif

        // run task
        try {
            task.first();
        } catch (std::bad_function_call& e) {
            // bad function
        }

        int64_t exec_cost = timer.tick();
        _counter.execute_delay += exec_cost;
        ++_counter.task_counter;
        if (_ctrl_waiting) {
            _ctrl_cond.notify_one();
        }
    }

#ifdef PERF_TEST
    std::lock_guard<std::mutex> perf_lock(g_perf_mutex);
    g_sched_delays.insert(g_sched_delays.end(), sched_delays.begin(), sched_delays.end());
#endif
}
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

