/**
 * @file instance_pool_test.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2019-02-15 13:30:22
 * @brief ∂‘œÛ≥ÿ≤‚ ‘≥Ã–Ú
 *
 **/

//#define __DEBUG_INSTANCE_POOL__
#define __FAST_FAIL__

#include <functional>
#include <iostream>
#include <mutex>
 
#include "atomic_array_queue.h"
#include "instance_pool.h"
#include "timer.h"
 
using namespace common;

static const size_t kInstancePoolCapacity = 32;
static const size_t kInstanceCount = 1003;
static const size_t kQueueLen = 64;
static const size_t kObjectSize = 128;

static const size_t kFetchThreadNum = 5;
static const size_t kGivebackThreadNum = 2;

struct TestStruct {
    char c_array[kObjectSize];
};

typedef AtomicArrayQueue<TestStruct> ObjectQueue;
typedef InstancePool<TestStruct> ObjectInstancePool;

ObjectInstancePool g_pool(kInstancePoolCapacity);
ObjectQueue g_queue(kQueueLen);

std::mutex g_print_mutex;

static void fetch_thread_func(size_t index) {
    size_t inst_cnt = kInstanceCount / kFetchThreadNum;
    size_t mod = kInstanceCount % kFetchThreadNum;
    if (index < mod) {
        ++inst_cnt;
    }
    for (size_t i = 0; i < inst_cnt; ++i) {
#ifdef __FAST_FAIL__
        TestStruct *inst = g_pool.fetch_fast_fail();
#else
        TestStruct *inst = g_pool.fetch();
#endif
        g_queue.push(inst);
    }

    std::lock_guard<std::mutex> lock(g_print_mutex);
    std::cout << "Fetch thread: " << index << ", count: " << inst_cnt << std::endl;
}
 
static void giveback_thread_func(size_t index) {
    size_t inst_cnt = kInstanceCount / kGivebackThreadNum;
    size_t mod = kInstanceCount % kGivebackThreadNum;
    if (index < mod) {
        ++inst_cnt;
    }
    for (size_t i = 0; i < inst_cnt; ++i) {
        TestStruct *inst = g_queue.pop();
        g_pool.give_back(inst);
    }

    std::lock_guard<std::mutex> lock(g_print_mutex);
    std::cout << "Giveback thread: " << index << ", count: " << inst_cnt << std::endl;
}
 
int main(int argc, char** argv) {
    std::thread* fetch_threads = new std::thread[kFetchThreadNum];
    std::thread* giveback_threads = new std::thread[kGivebackThreadNum];

    MicrosecondsTimer timer;
    for (size_t i = 0; i < kFetchThreadNum; ++i) {
        std::thread t(std::bind(&fetch_thread_func, i));
        fetch_threads[i].swap(t);
    }

    std::this_thread::sleep_for(Milliseconds(1));
    for (size_t i = 0; i < kGivebackThreadNum; ++i) {
        std::thread t(std::bind(&giveback_thread_func, i));
        giveback_threads[i].swap(t);
    }

    for (size_t i = 0; i < kFetchThreadNum; ++i) {
        fetch_threads[i].join();
    }
    for (size_t i = 0; i < kGivebackThreadNum; ++i) {
        giveback_threads[i].join();
    }
    std::cout << "Fetch count: " << kInstanceCount << ", Cost: " << timer.tick() << "us" << std::endl;

    delete[] fetch_threads;
    delete[] giveback_threads;
    return 0;
}
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

