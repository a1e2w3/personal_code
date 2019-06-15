/**
 * @file thread_pool.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 17:27:55
 * @brief
 *
 **/

#pragma once
 
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <thread>
#include <tuple>

#include "thread_pool/task_queue.h"
 
namespace common {
    
typedef std::tuple<uint64_t, double, double> PerfInfo;

void print_sched_pretiles();

class ThreadPool {
public:
	ThreadPool(uint32_t thread_num);
	~ThreadPool();

	bool start(TaskQueue* queue);
	bool stop(bool wait = false);

	bool is_running() const {
	    return _is_running.load();
	}

    PerfInfo profile(bool clear = false) {
	    return _counter.get(clear);
	}

    uint32_t thread_num() const {
        return _thread_num;
    }

private:
	void thread_run_wrapper(size_t thread_index);

	const uint32_t _thread_num;
    std::mutex _ctrl_mutex;  // mutex for start/stop ctrl
	std::condition_variable _ctrl_cond; // cond for ctrl
	TaskQueue* _queue;
    std::unique_ptr<std::thread[]> _threads;
	
    std::atomic<bool> _stop;
	std::atomic<bool> _is_running;
    std::atomic<bool> _ctrl_waiting;

    struct PerfCounter {
	    std::atomic<uint64_t> task_counter;
        std::atomic<uint64_t> schedule_delay;
        std::atomic<uint64_t> execute_delay;

        static uint64_t get_value(std::atomic<uint64_t>& counter, bool clear) {
            return clear ? counter.exchange(0) : counter.load();
        }

        PerfInfo get(bool clear) {
            uint64_t task_cnt = get_value(task_counter, clear);
            if (task_cnt == 0) {
                schedule_delay.store(0);
                execute_delay.store(0);
                return std::make_tuple(task_cnt, NAN, NAN);
            }
            double sche_delay = static_cast<double>(get_value(schedule_delay, clear)) / task_cnt;
            double exec_delay = static_cast<double>(get_value(execute_delay, clear)) / task_cnt;
            return std::make_tuple(task_cnt, sche_delay, exec_delay);
        }
    };

    PerfCounter _counter;
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

