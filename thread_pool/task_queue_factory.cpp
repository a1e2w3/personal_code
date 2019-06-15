/**
 * @file task_queue_factory.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 17:11:15
 * @brief
 *  
 **/
 
#include "thread_pool/task_queue_factory.h"

namespace common {
 
TaskQueue* TaskQueueFactory::new_instance(const std::string& queue_name) {
    return TaskQueueRegister::get_instance().get_creator(queue_name)();
}

static TaskQueue* null_creator() {
    return nullptr;
}

TaskQueueCreator TaskQueueRegister::get_creator(const std::string& queue_name) {
    auto iter = _creator_map.find(queue_name);
    if (iter == _creator_map.end()) {
        return null_creator;
    } else {
        return iter->second;
    }
}
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

