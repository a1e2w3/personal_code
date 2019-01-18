/**
 * @file background.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-05 18:05:14
 * @brief 后台定时任务管理
 *  
 **/
 
#ifndef WRPC_UTILS_BACKGROUND_H_
#define WRPC_UTILS_BACKGROUND_H_
 
#include <memory>

#include "task.h"
 
namespace wrpc {

typedef common::TaskId BackgroundTaskId;

const BackgroundTaskId INVALID_TASK_ID = common::kInvalidId;

BackgroundTaskId add_background_task(const common::TaskFunc& func, uint64_t delay_ms = 0);

bool cancel_background_task(const BackgroundTaskId& id);

class PeriodicTaskController;
typedef std::shared_ptr<PeriodicTaskController> PeriodicTaskControllerPtr;

// 周期性任务控制器
class PeriodicTaskController : public std::enable_shared_from_this<PeriodicTaskController> {
private:
    PeriodicTaskController();
    ~PeriodicTaskController();

public:
    static PeriodicTaskControllerPtr make_controller() {
        return PeriodicTaskControllerPtr(
                new (std::nothrow) PeriodicTaskController(),
                PeriodicTaskController::delete_controller);
    }

    bool start(const common::TaskFunc& func, uint64_t interval_ms);

    void cancel();

private:
    static void delete_controller(PeriodicTaskController* controller) {
    	if (controller != nullptr) {
            delete controller;
    	}
    }

    typedef std::weak_ptr<PeriodicTaskController> PeriodicTaskControllerWeakPtr;
    static void periodic_task_wrapper(const common::TaskFunc& func,
            uint64_t interval_ms, PeriodicTaskControllerWeakPtr controller);

private:
    BackgroundTaskId _cur_task_id;
    bool _switch;
};

} // end namespace wrpc

#endif // WRPC_UTILS_BACKGROUND_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

