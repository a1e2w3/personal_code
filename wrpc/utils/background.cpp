/**
 * @file background.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-05 18:05:55
 * @brief 
 *  
 **/
 
#include "utils/background.h"

#include <mutex>
#include "thread_pool.h"
#include "timer_task_queue.h"

#include "utils/common_flags.h"
#include "utils/write_log.h"
 
namespace wrpc {

static common::ThreadPool *g_bg_threads = nullptr;
static common::TimerTaskQueue g_task_queue;
static std::once_flag g_init_bg_threads_once;

static void stop_thread_pool() {
    if (g_bg_threads != nullptr) {
        DEBUG("stop bg threads");
        g_bg_threads->stop(false);
        delete g_bg_threads;
        g_bg_threads = nullptr;
    }
}

static void start_thread_pool() {
    g_bg_threads = new common::ThreadPool(WRPC_BACKGROUND_THREAD_NUMS);
    g_bg_threads->start(&g_task_queue);
    atexit(stop_thread_pool);
}
 
BackgroundTaskId add_background_task(const common::TaskFunc& func, uint64_t delay_ms) {
    std::call_once(g_init_bg_threads_once, start_thread_pool);
    if (g_bg_threads != nullptr) {
        // 防止后台线程停止后(可能任务队列也被析构), 继续往队列插入任务
    	// 否则可能触发死循环: 任务队列析构清空队列 → 释放任务持有的资源 →
    	//              资源上报被析构(如request取消后feedback) → 继续往队列插入任务
        return g_task_queue.push_delay_task(delay_ms * 1000, func);
    }
    return INVALID_TASK_ID;
}

bool cancel_background_task(const BackgroundTaskId& id) {
    if (g_bg_threads != nullptr) {
        // 后台线程停止后(可能任务队列也被析构), 不应再操作任务队列
        return g_task_queue.cancel_task(id);
    }
    return false;
}

PeriodicTaskController::PeriodicTaskController()
    : _cur_task_id(INVALID_TASK_ID), _switch(false) {}

PeriodicTaskController::~PeriodicTaskController() {
    cancel();
}

bool PeriodicTaskController::start(const common::TaskFunc& func, uint64_t interval_ms) {
    if (_switch) {
        // running
        return false;
    }
    _switch = true;
    _cur_task_id = add_background_task(
            std::bind(&PeriodicTaskController::periodic_task_wrapper, func, interval_ms, shared_from_this()),
            interval_ms);
    DEBUG("start periodic task: %ld", _cur_task_id);
    return _cur_task_id != INVALID_TASK_ID;
}

void PeriodicTaskController::cancel() {
    if (_switch) {
        _switch = false;
        DEBUG("cancel periodic task: %ld", _cur_task_id);
        cancel_background_task(_cur_task_id);
        _cur_task_id = INVALID_TASK_ID;
    }
}

void PeriodicTaskController::periodic_task_wrapper(const common::TaskFunc& func,
        uint64_t interval_ms, PeriodicTaskControllerWeakPtr controller) {
	PeriodicTaskControllerPtr locked = controller.lock();
    if (locked && locked->_switch) {
        DEBUG("run periodic task: %ld", locked->_cur_task_id);
        func();
        if (locked->_switch) {
        	locked->_cur_task_id = add_background_task(
                std::bind(&PeriodicTaskController::periodic_task_wrapper, func, interval_ms, controller),
                interval_ms);
            DEBUG("repush periodic task: %ld", locked->_cur_task_id);
        }
    }
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

