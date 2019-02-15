/**
 * @file event_dispatcher.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-05 15:33:53
 * @brief 
 *  
 **/
 
#include "network/event_dispatcher.h"

#include <assert.h>
#include <functional>
#include <mutex>
#include <stdlib.h>
#include <sys/epoll.h>

#include "network/controller.h"
#include "utils/common_flags.h"
#include "utils/write_log.h"
 
namespace wrpc {

EventDispatcher::EventDispatcher() : _epoll_fd(-1), _stop(false), _is_running(false) {
}

EventDispatcher::~EventDispatcher() {
    stop(true);
}

int EventDispatcher::start() {
    if (_dispatch_thread) {
        WARNING("EventDispatcher is running already");
        return -1;
    }

    _epoll_fd = epoll_create(1024 * 1024);
    if (_epoll_fd < 0) {
        FATAL("Fail to create epoll, ret: %d", _epoll_fd);
        return _epoll_fd;
    }

    _wakeup_fds[0] = -1;
    _wakeup_fds[1] = -1;
    int ret = pipe(_wakeup_fds);
    if (ret != 0) {
        FATAL("Fail to create pipe, ret: %d", ret);
        close(_epoll_fd);
        _epoll_fd = -1;
        return ret;
    }

    _dispatch_thread.reset(new std::thread(
            std::bind(&EventDispatcher::thread_run_wrapper, this)));
    return 0;
}

void EventDispatcher::stop(bool wait) {
    if (!is_running()) {
        return;
    }
    _stop.store(true);

    // wakeup epoll wait
    if (_epoll_fd >= 0) {
        epoll_event evt = { EPOLLOUT,  { NULL } };
        epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _wakeup_fds[1], &evt);
    }

    if (wait) {
        join();
    }
}

bool EventDispatcher::is_running() const {
    return (_epoll_fd >= 0 && _dispatch_thread.get() != nullptr && !_stop.load());
}

void EventDispatcher::join() {
    if (_dispatch_thread) {
        _dispatch_thread->join();
        _dispatch_thread.reset();
        release_fd();
    }
}

int EventDispatcher::add_listener(ControllerId cid, int fd) {
    if (cid == INVALID_CONTROLLER_ID || fd < 0) {
        return -1;
    }
    if (_epoll_fd < 0) {
        errno = EINVAL;
        return -1;
    }
    epoll_event evt;
    evt.events = EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;
    ListenerData *data = (ListenerData *) (&evt.data.u64);
    data->cid = cid;
    data->fd = fd;
    DEBUG("event dispacher add controller:%lu, fd:%d", cid, fd);
    return epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &evt);
}

int EventDispatcher::remove_listener(int fd) {
    if (fd < 0) {
        return -1;
    }
    if (_epoll_fd < 0) {
        errno = EINVAL;
        return -1;
    }
    DEBUG("event dispacher remove fd:%d", fd);
    return epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void EventDispatcher::release_fd() {
    if (_epoll_fd >= 0) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
    if (_wakeup_fds[0] > 0) {
        close(_wakeup_fds[0]);
        close(_wakeup_fds[1]);
        _wakeup_fds[0] = -1;
        _wakeup_fds[1] = -1;
    }
}

void EventDispatcher::thread_run_wrapper() {
    DEBUG("start event dispacher");
    const uint32_t EXPECTED_EVENTS = (EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
    size_t event_size = WRPC_EVENT_DISPATCHER_EVENT_SIZE;
    epoll_event e[event_size];
    while (!_stop.load()) {
#ifdef WRPC_ADDITIONAL_EPOLL
        // Performance downgrades in examples.
        int n = epoll_wait(_epoll_fd, e, event_size, 0);
        if (n == 0) {
            n = epoll_wait(_epoll_fd, e, event_size, -1);
        }
#else
        const int n = epoll_wait(_epoll_fd, e, event_size, -1);
#endif
        if (_stop.load()) {
            // epoll_ctl/epoll_wait should have some sort of memory fencing
            // guaranteeing that we(after epoll_wait) see _stop set before
            // epoll_ctl.
            break;
        }
        if (n < 0) {
            if (EINTR == errno) {
                // We've checked _stop, no wake-up will be missed.
                continue;
            }
            FATAL("Fail to epoll_wait fd = %d", _epoll_fd);
            break;
        }
        for (int i = 0; i < n; ++i) {
            if (e[i].events & EXPECTED_EVENTS) {
                notify((const ListenerData *) (&e[i].data.u64), e[i].events);
                DEBUG("notify epoll event: %d %d %lu", i, e[i].events, e[i].data.u64);
            }
        }
    }
    DEBUG("event dispacher stopped");
}

void EventDispatcher::notify(const ListenerData *data, uint32_t epoll_events) {
    assert(data != nullptr);
    ControllerPtr controller = ControllerAddresser::address(data->cid).lock();
    if (!controller) {
        WARNING("listener data %lu not found controller:%lu, ignore event", data->u64, data->cid);
        return;
    }

    if (epoll_events & EPOLLIN) {
        DEBUG("notify epoll in controller:%lu, fd:%d", data->cid, data->fd);
        controller->on_epoll_in(data->fd);
    } else {
        DEBUG("notify epoll error controller:%lu, fd:%d", data->cid, data->fd);
        controller->on_epoll_error(data->fd);
    }
}

static EventDispatcher g_disps[WRPC_EVENT_DISPATCHER_NUMS];
static std::once_flag g_init_disp_once;

static void stop_event_dispatchers() {
    size_t num = WRPC_EVENT_DISPATCHER_NUMS;
    if (nullptr != g_disps) {
        for (size_t i = 0; i < num; ++i) {
            g_disps[i].stop(false);
        }
        for (size_t i = 0; i < num; ++i) {
            g_disps[i].join();
        }
    }
    return;
}

static void init_event_dispatchers() {
    size_t num = WRPC_EVENT_DISPATCHER_NUMS;
    for (size_t i = 0; i < num; ++i) {
        if (0 != g_disps[i].start()) {
            FATAL("Start event dispatcher %lu failed.", i);
            return;
        }
    }
    atexit(stop_event_dispatchers);
}

EventDispatcher& get_event_dispatcher(int fd) {
    std::call_once(g_init_disp_once, init_event_dispatchers);
    size_t num = WRPC_EVENT_DISPATCHER_NUMS;
    if (num == 1) {
        return g_disps[0];
    } else {
        return g_disps[static_cast<size_t>(fd) % num];
    }
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

