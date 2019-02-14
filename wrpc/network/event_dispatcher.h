/**
 * @file event_dispatcher.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-05 15:33:49
 * @brief 
 *   event loop thread based on epoll
 **/
 
#ifndef WRPC_NETWORK_EVENT_DISPATCHER_H_
#define WRPC_NETWORK_EVENT_DISPATCHER_H_
 
#include <atomic>
#include <memory>
#include <thread>

#include "network/network_common.h"
#include "utils/common_define.h"
 
namespace wrpc {

class EventDispatcher {
public:
	EventDispatcher();
    ~EventDispatcher();
    // disallow copy
	EventDispatcher(const EventDispatcher&) = delete;
	EventDispatcher& operator = (const EventDispatcher&) = delete;

    int start();

    void stop(bool wait = true);

    bool is_running() const;

    void join();

    int add_listener(ControllerId cid, int fd);

    int remove_listener(int fd);

private:
    void thread_run_wrapper();
    void notify(const ListenerData *data, uint32_t epoll_events);
    void release_fd();

private:
    int _epoll_fd;
    std::atomic<bool> _stop;
    std::atomic<bool> _is_running;
    std::unique_ptr<std::thread> _dispatch_thread;

    // Pipe fds to wakeup EventDispatcher from `epoll_wait' in order to quit
    int _wakeup_fds[2];
};

EventDispatcher& get_event_dispatcher(int fd);
 
} // end namespace wrpc
 
#endif // WRPC_NETWORK_EVENT_DISPATCHER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

