/**
 * @file task_queue_factory.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-28 17:11:12
 * @brief 任务队列工厂
 *  
 **/
 
#pragma once

#include <map>
#include <string>

#include "task_queue.h"
 
namespace common {

typedef TaskQueue* (*TaskQueueCreator) ();

template<class QueueType>
TaskQueue* create_queue() {
    return new QueueType();
}
 
class TaskQueueFactory {
public:
	TaskQueueFactory() {}
	~TaskQueueFactory() {}

	TaskQueue* new_instance(const std::string& queue_name);
};

class TaskQueueRegister {
public:
	~TaskQueueRegister() {}

	static TaskQueueRegister& get_instance() {
	    static TaskQueueRegister _register;
	    return _register;
	}

	void register_queue(const std::string& queue_name, TaskQueueCreator creator) {
	    _creator_map[queue_name] = creator;
	}

	TaskQueueCreator get_creator(const std::string& queue_name);

private:
	TaskQueueRegister() {}
	std::map<std::string, TaskQueueCreator> _creator_map;
};
 
} // end namespace common

#define REGISTER_QUEUE(name, creator) \
    static bool __register_##name() { \
	    common::TaskQueueRegister::get_instance().register_queue(#name, creator); \
	    return true; \
    } \
    extern bool __is_##name##_registered; \
	bool __is_##name##_registered = __register_##name(); \
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

