/**
 * @file main.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 13:44:16
 * @brief œﬂ≥Ã≥ÿ≤‚ ‘≥Ã–Ú
 *  
 **/

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdlib.h>
#include <sys/time.h>
#include <thread>
#include <vector> 

#include "task_queue_factory.h"
#include "thread_pool.h"
#include "timer.h"

using namespace common;

static const size_t kTestThreadNum = 128;
static const size_t kTestTaskCount = 100000;
static const size_t kTaskSleepMs = 10;

static std::atomic<bool> print(true);

static void task_wrapper() {
	// just sleep 10ms
	std::this_thread::sleep_for(Milliseconds(10));
}
#ifdef DEBUG
static void print_queue_len(TaskQueue* queue, ThreadPool* thread_pool) {
    static size_t total_tasks = 0;
    
    size_t len = queue->queue_len();
    auto profile = thread_pool->profile(true);
    total_tasks += std::get<0>(profile);
    std::cout << "queue len: " << len
    		  << ", run tasks: " << std::get<0>(profile)
              << ", sched: " << std::get<1>(profile)
              << ", exec: " << std::get<2>(profile)
    		  << ", total run: " << total_tasks << std::endl;
}

static void print_thread_wrapper(TaskQueue* queue, ThreadPool* thread_pool) {
    while (print) {
        std::this_thread::sleep_for(Seconds(1));
        print_queue_len(queue, thread_pool);
    }
}
#else
static void print_queue_len(TaskQueue*, ThreadPool*) {}
static void print_thread_wrapper(TaskQueue*, ThreadPool*) {}
#endif

static inline uint8_t rand_priority() {
    srand(get_micro());
    return static_cast<uint16_t>(rand()) & 0xff;
}

static void pressure_test(TaskQueue* queue, ThreadPool* thread_pool) {
    std::cout << "Pressure Test..." << std::endl;
    uint64_t cost = 0;
    TaskAttr attr;
    print.store(true);
    std::thread print_thread(std::bind(&print_thread_wrapper, queue, thread_pool));

    TIMER(cost) {
        thread_pool->start(queue);
        for (size_t i = 0; i < kTestTaskCount; ++i) {
        	attr.priority = rand_priority();
            attr.exec_time = get_micro();
            queue->push_task(task_wrapper, attr);
        }
        thread_pool->stop(true);
    }

    print.store(false);
    print_thread.join();
    print_queue_len(queue, thread_pool);
    
    std::cout << "Task cnt: " << kTestTaskCount << ", Thread num: " << kTestThreadNum
    		  << ", Cost: " << cost << "us." << std::endl;
    print_sched_pretiles();
    std::cout << std::endl;
}

static void performance_test(TaskQueue* queue, ThreadPool* thread_pool) {
    std::cout << "Performance Test..." << std::endl;
    uint64_t cost = 0;
    TaskAttr attr;
    size_t sleep_us = kTaskSleepMs * 1000 * 4 / kTestThreadNum / 5;
    print.store(true);
    std::thread print_thread(std::bind(&print_thread_wrapper, queue, thread_pool));

    TIMER(cost) {
        thread_pool->start(queue);
        for (size_t i = 0; i < kTestTaskCount; ++i) {
            // control push speed
            attr.priority = rand_priority();
            attr.exec_time = get_micro();
            queue->push_task(task_wrapper, attr);
            std::this_thread::sleep_for(Microseconds(sleep_us));
        }
        thread_pool->stop(true);
    }

    print.store(false);
    print_thread.join();
    print_queue_len(queue, thread_pool);
    
    std::cout << "Task cnt: " << kTestTaskCount << ", Thread num: " << kTestThreadNum
    		  << ", Cost: " << cost << "us." << std::endl;
    print_sched_pretiles();
    std::cout << std::endl;
}
 
int main(int argc, char** argv) {
	TaskQueueFactory factory;
    ThreadPool* thread_pool = new ThreadPool(kTestThreadNum);

	const char* queue_name = "fifo_queue";
	if (argc > 1) {
		queue_name = argv[1];
	}

	TaskQueue* queue = factory.new_instance(queue_name);
	if (queue == nullptr) {
	    std::cerr << "unknow task queue: " << queue_name << std::endl;
        delete thread_pool;
	    return -1;
	}

    performance_test(queue, thread_pool);
    delete queue;
    queue = factory.new_instance(queue_name);
    
    std::this_thread::sleep_for(Seconds(1));
    pressure_test(queue, thread_pool);
    delete queue;
    delete thread_pool;
    return 0;
}
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

