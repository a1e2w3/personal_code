/***************************************************************************
 * 
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file controller.cpp
 * @author wangcong07(wangcong07@baidu.com)
 * @date 2018-01-26 12:55:46
 * @brief 
 *  
 **/
 
#include "network/controller.h"

#include "network/channel.h"
#include "network/connection.h"
#include "network/event_dispatcher.h"
#include "utils/write_log.h"

namespace wrpc {

static inline void cancel_bg_task(BackgroundTaskId& task_id) {
    if (task_id != INVALID_TASK_ID) {
        cancel_background_task(task_id);
        task_id = INVALID_TASK_ID;
    }
}

static inline int status_2_ret_code(ControllerStatus status) {
    switch (status) {
        case SUCCESS:
            return NET_SUCC;
        case TIMEOUT:
            return NET_TIMEOUT;
        case CANCELED:
            return NET_CANCELED;
        default:
            return NET_INTERNAL_ERROR;
    }
}

RequestController::RequestController(Controller* controller)
    : _is_running(false),
      _controller(controller),
      _response(nullptr),
      _error_code(NET_UNKNOW_ERROR),
      _try_count(0) {
    _response = _controller->_channel->make_response();
}

RequestController::~RequestController() {
    cancel(NET_CANCELED);
    _controller->_channel->destroy_response(_response);
}

int RequestController::get_fd() const {
    return _connection ? _connection->get_fd() : -1;
}

void RequestController::feedback(int code) {
    _feedback_info.code = code;
    _feedback_info.total_cost = _timer.tick();
    _controller->feedback(_feedback_info);
    if (code == NET_SUCC || code == NET_BACKUP_SUCCESS || code == NET_CANCELED) {
        NOTICE_LIGHT("logid[%s] try[%u] endpoint[%s] code[%d] cost[%u] conn[%u] write[%u] read[%u]",
            _controller->_logid.c_str(), _try_count, _feedback_info.endpoint.to_string().c_str(), 
            _feedback_info.code, _feedback_info.total_cost, _feedback_info.connect_cost, 
            _feedback_info.write_cost, _feedback_info.read_cost);
    } else {
        WARNING("logid[%s] try[%u] endpoint[%s] code[%d] cost[%u] conn[%u] write[%u] read[%u]",
            _controller->_logid.c_str(), _try_count, _feedback_info.endpoint.to_string().c_str(), 
            _feedback_info.code, _feedback_info.total_cost, _feedback_info.connect_cost, 
            _feedback_info.write_cost, _feedback_info.read_cost);
    }
}

int RequestController::issue_rpc() {
    if (_controller->_request == nullptr || _response == nullptr) {
        _error_code = NET_INTERNAL_ERROR;
        return _error_code;
    }
    // fetch connection
    _try_count = _controller->_retry_count;
    _is_running = true;
    _timer.reset();
    _error_code = _controller->fetch_connection(_connection);
    if (_error_code != NET_SUCC || !_connection) {
        return retry(_error_code);
    }

    _feedback_info.reset(_connection->end_point());
    _feedback_info.connect_cost = _timer.tick();
    // check total timeout
    MillisecondsCountdownTimer::rep_type remain;
    if(_controller->_timer.timeout(&remain)) {
        _error_code = NET_TIMEOUT;
        feedback(NET_TIMEOUT);
        _controller->giveback_connection(std::move(_connection), true);
        return NET_TIMEOUT;
    }

    // send request
    TIMER(_feedback_info.write_cost) {
        _error_code = _controller->_request->write_to(_connection.get(), remain);
    }
    if (_error_code != NET_SUCC) {
        _controller->giveback_connection(std::move(_connection), true);
        return retry(_error_code);
    }
    // check total timeout
    if(_controller->_timer.timeout()) {
        _error_code = NET_TIMEOUT;
        feedback(NET_TIMEOUT);
        _controller->giveback_connection(std::move(_connection), true);
        return NET_TIMEOUT;
    }

    // add listener
    int ret = listen();
    if (ret != NET_SUCC) {
        _error_code = NET_EPOLL_FAIL;
        unlisten();
        _controller->giveback_connection(std::move(_connection), true);
        return retry(_error_code);
    }
    return NET_SUCC;
}

int RequestController::listen() {
    if (_connection) {
        EventDispatcher& dispatcher = get_event_dispatcher(_connection->get_fd());
        return dispatcher.add_listener(_controller->_id, _connection->get_fd());
    } else {
        return NET_DISCONNECTED;
    }
}

int RequestController::unlisten() {
    if (_connection) {
        EventDispatcher& dispatcher = get_event_dispatcher(_connection->get_fd());
        return dispatcher.remove_listener(_connection->get_fd());
    } else {
        return NET_DISCONNECTED;
    }
}

int RequestController::retry(int ret_code) {
    feedback(ret_code);
    if (!need_retry(ret_code) || !_controller->check_need_retry()) {
        _controller->giveback_connection(std::move(_connection), true);
        _is_running = false;
        return ret_code;
    } else {
        ++(_controller->_retry_count);
        DEBUG("logid:%s retry %u times.", _controller->_logid.c_str(), _controller->_retry_count);
        return issue_rpc();
    }
}

void RequestController::cancel(int code) {
    if (!is_running()) {
        return;
    }

    DEBUG("logid:%s cancel request with code: %d", _controller->_logid.c_str(), code);
    _is_running = false;
    unlisten();
    _controller->giveback_connection(std::move(_connection), true);
    _error_code = code;
    feedback(code);
}

int RequestController::on_epoll_in() {
    unlisten();
    MillisecondsCountdownTimer::rep_type remain;
    if (_controller->_timer.timeout(&remain)) {
        _controller->giveback_connection(std::move(_connection), true);
        _error_code = NET_TIMEOUT;
        feedback(NET_TIMEOUT);
        return NET_TIMEOUT;
    } else {
        int ret = NET_SUCC;
        TIMER(_feedback_info.read_cost) {
            ret = _response->read_from(_connection.get(), remain);
        }
        _error_code = ret;
        if (ret == NET_SUCC) {
            // rpc success
            _controller->giveback_connection(std::move(_connection), false);
            _is_running = false;
            feedback(NET_SUCC);
            DEBUG("logid:%s receive response success.", _controller->_logid.c_str());
            return NET_SUCC;
        } else {
            feedback(ret);
            DEBUG("logid:%s receive response failed. ret:%d", _controller->_logid.c_str(), ret);
            retry(ret);
            return ret;
        }
    }
}

int RequestController::on_epoll_error() {
    unlisten();
    _controller->giveback_connection(std::move(_connection), true);
    if (_controller->_timer.timeout()) {
        _error_code = NET_TIMEOUT;
        feedback(NET_TIMEOUT);
        return NET_TIMEOUT;
    } else {
        WARNING("logid:%s epoll error", _controller->_logid.c_str());
        _error_code = NET_EPOLL_FAIL;
        feedback(NET_EPOLL_FAIL);
        retry(NET_EPOLL_FAIL);
        return NET_EPOLL_FAIL;
    }
}

Controller::Controller(ChannelPtr&& channel, const RPCOptions& options)
    : _id(INVALID_CONTROLLER_ID),
      _options(options),
      _timer(-1),
      _channel(channel),
      _request(nullptr),
      _response(nullptr),
      _error_code(NET_UNKNOW_ERROR),
      _cost(0),
      _use_debug_ep(false),
      _status(INIT),
      _retry_count(0),
      _has_backup_request(false),
      _submit_task_id(INVALID_TASK_ID),
      _timeout_task_id(INVALID_TASK_ID),
      _backup_request_task_id(INVALID_TASK_ID),
      _user_thread_joining(0),
      _pending_tasks(),
      _self(nullptr) {
    _request = _channel->make_request();
}

Controller::~Controller() {
    DEBUG("logid:%s deconstruct controller", _logid.c_str());
    ControllerAddresser::remove(_id);
    _response = nullptr;
    // ensure deconstruct before _channel
    _normal_request.reset();
    _backup_request.reset();

    // should not run callback in deconstructor
    cancel(false);
    _channel->destroy_request(_request);
}

const std::string& Controller::get_protocol() const {
    return _channel->_options.protocol;
}

int Controller::submit() {
    return submit(nullptr);
}

int Controller::submit(const RPCCallback& callback) {
    if (nullptr == _request) {
        return NET_INTERNAL_ERROR;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _submit_task_id = INVALID_TASK_ID;

    // check status
    if (_status != INIT && _status != SUBMITING) {
        return NET_INVALID_ARGUMENT;
    }

    // register firstly
    _id = ControllerAddresser::regist(shared_from_this());

    _status = RUNNING;
    _user_callback = callback;
    _timer.reset(_options.total_timeout_ms);
    _normal_request.reset(new RequestController(this));
    int ret = _normal_request->issue_rpc();
    if (ret != NET_SUCC) {
        on_rpc_failed(_normal_request->error_code());
        return ret;
    }

    // set background timer tasks
    if (_options.total_timeout_ms >= 0) {
        _timeout_task_id = add_background_task(
                std::bind(&Controller::controller_timeout_wrapper, shared_from_this()),
                _timer.remain());
    }
    if (_options.backup_request_timeout_ms > 0) {
        int32_t delay_time = _options.backup_request_timeout_ms;
        if (_options.total_timeout_ms >= 0 &&
                _options.total_timeout_ms < _options.backup_request_timeout_ms) {
            delay_time = _options.total_timeout_ms;
        }

        delay_time -= _timer.tick();
        _backup_request_task_id = add_background_task(
                std::bind(&Controller::backup_request_wrapper, shared_from_this()),
                std::max(0, delay_time));
    }
    return ret;
}

void Controller::submit_wrapper(ControllerWeakPtr controller, const RPCCallback& callback) {
    ControllerPtr lock_controller = controller.lock();
    if (lock_controller) {
        lock_controller->submit(callback);
    }
}

int Controller::submit_async() {
    return submit_async(nullptr);
}

int Controller::submit_async(const RPCCallback& callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_status != INIT) {
        return NET_INVALID_ARGUMENT;
    }
    _status = SUBMITING;
    _submit_task_id = add_background_task(std::bind(&Controller::submit_wrapper,
            shared_from_this(), callback));
    return NET_SUCC;
}

static void feedback_wrapper(ChannelWeakPtr channel, FeedbackInfo info) {
    ChannelPtr lock_channel = channel.lock();
    if (lock_channel) {
        lock_channel->feedback(info);
    }
}

void Controller::feedback(const FeedbackInfo& info) {
    // run feedback in background threads
    add_background_task(std::bind(&feedback_wrapper, _channel, info));
}

int Controller::fetch_connection(ConnectionPtr& connection) {
    if (_use_debug_ep) {
        // just connect debug end point
        connection.reset(new Connection(_debug_ep));
        return connection->connect(_options.connect_timeout_ms);
    }

    _lb_context.retry_count = _retry_count;
    _lb_context.hash_code = _request->hash_code();
    return _channel->fetch_connection(_lb_context, _options.connect_timeout_ms, connection);
}

void Controller::giveback_connection(ConnectionPtr&& connection, bool close) {
    _channel->giveback_connection(std::move(connection), close);
}

void Controller::on_epoll_in(int fd) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_user_thread_joining > 0) {
        // running in user thread
        DEBUG("logid: %s on_epoll_in in user thread", _logid.c_str());
        _pending_tasks.emplace_back(std::bind(
                &Controller::handle_epoll_in, this, fd));
        _cond.notify_one();
    } else {
        // running in back ground threads
        DEBUG("logid: %s on_epoll_in in background thread", _logid.c_str());
        BackgroundTaskId task_id = add_background_task(std::bind(
                &Controller::epoll_in_wrapper, shared_from_this(), fd));
        _bg_tasks.push_back(task_id);
    }
}

void Controller::on_epoll_error(int fd) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_user_thread_joining > 0) {
        // running in user thread
        DEBUG("logid: %s on_epoll_error in user thread", _logid.c_str());
        _pending_tasks.emplace_back(std::bind(
                &Controller::handle_epoll_in, this, fd));
        _cond.notify_one();
    } else {
        // running in back ground threads
        DEBUG("logid: %s on_epoll_error in background thread", _logid.c_str());
        BackgroundTaskId task_id = add_background_task(std::bind(
                &Controller::epoll_error_wrapper, shared_from_this(), fd));
        _bg_tasks.push_back(task_id);
    }
}

void Controller::epoll_in_wrapper(ControllerWeakPtr controller, int fd) {
    ControllerPtr lock_controller = controller.lock();
    if (lock_controller) {
        std::lock_guard<std::mutex> lock(lock_controller->_mutex);
        lock_controller->handle_epoll_in(fd);
    }
}
void Controller::epoll_error_wrapper(ControllerWeakPtr controller, int fd) {
    ControllerPtr lock_controller = controller.lock();
    if (lock_controller) {
        std::lock_guard<std::mutex> lock(lock_controller->_mutex);
        lock_controller->handle_epoll_error(fd);
    }
}

void Controller::handle_epoll_in(int fd) {
    if (_status != RUNNING) {
        // not running, ignore
        return;
    }

    if (_timer.timeout()) {
        on_rpc_timeout();
        return;
    }

    if (_normal_request && _normal_request->get_fd() == fd) {
        DEBUG("logid: %s handle_epoll_in fd:%d normal request", _logid.c_str(), fd);
        int ret = _normal_request->on_epoll_in();
        if (ret == NET_SUCC) {
            // read response success
            _response = _normal_request->get_response();
            // cancel backup request
            if (_backup_request) {
                _backup_request->cancel(NET_BACKUP_SUCCESS);
                _backup_request.reset();
            }
            on_rpc_success();
        } else {
            // read response failed
            if (!normal_request_running() && !backup_request_running()) {
                on_rpc_failed(_normal_request->error_code());
            }
        }
    } else if (_backup_request && _backup_request->get_fd() == fd) {
        DEBUG("logid: %s handle_epoll_in fd:%d backup request", _logid.c_str(), fd);
        int ret = _backup_request->on_epoll_in();
        if (ret == NET_SUCC) {
            // read response success
            _response = _backup_request->get_response();
            // cancel normal request
            if (_normal_request) {
                _normal_request->cancel(NET_BACKUP_SUCCESS);
                _normal_request.reset();
            }
            on_rpc_success();
        } else {
            // read response failed
            if (!normal_request_running() && !backup_request_running()) {
                on_rpc_failed(_backup_request->error_code());
            }
        }
    } else {
        // unknow fd, remove listener and ignore
        WARNING("logid: %s handle_epoll_in for unknow fd:%d", _logid.c_str(), fd);
        EventDispatcher& dispatcher = get_event_dispatcher(fd);
        dispatcher.remove_listener(fd);
    }
}

void Controller::handle_epoll_error(int fd) {
    if (_status != RUNNING) {
        // not running, ignore
        return;
    }

    if (_timer.timeout()) {
        on_rpc_timeout();
        return;
    }

    int err_code = NET_SUCC;
    if (_normal_request && _normal_request->get_fd() == fd) {
        DEBUG("logid: %s handle_epoll_error fd:%d normal request", _logid.c_str(), fd);
        _normal_request->on_epoll_error();
        err_code = _normal_request->error_code();
    } else if (_backup_request && _backup_request->get_fd() == fd) {
        DEBUG("logid: %s handle_epoll_error fd:%d backup request", _logid.c_str(), fd);
        _backup_request->on_epoll_error();
        err_code = _backup_request->error_code();
    } else {
        // unknow fd, remove listener and ignore
        WARNING("logid: %s handle_epoll_error for unknow fd:%d", _logid.c_str(), fd);
        EventDispatcher& dispatcher = get_event_dispatcher(fd);
        dispatcher.remove_listener(fd);
        return;
    }

    if (!normal_request_running() && !backup_request_running()) {
        on_rpc_failed(err_code);
    }
}

void Controller::cleanup() {
    ControllerAddresser::remove(_id);
    cancel_bg_task(_submit_task_id);
    cancel_bg_task(_backup_request_task_id);
    cancel_bg_task(_timeout_task_id);
    cancel_pending_bg_tasks();
}

void Controller::on_rpc_success() {
    if (_status != RUNNING) {
        return;
    }
    _cost = _timer.tick();
    _status = SUCCESS;
    _error_code = NET_SUCC;
    cleanup();
    DEBUG("logid:%s controller success, retry_count:%u", _logid.c_str(), _retry_count);
    if (_user_callback) {
        _user_callback(shared_from_this(), _request, _response);
    }
    _self.reset();
    _cond.notify_all();
}

void Controller::on_rpc_timeout() {
    if (_status != RUNNING) {
        return;
    }

    _cost = _timer.tick();
    _status = TIMEOUT;
    _error_code = NET_TIMEOUT;
    _response = nullptr;
    cleanup();

    if (_backup_request) {
        _backup_request->cancel(NET_TIMEOUT);
        _backup_request.reset();
    }
    if (_normal_request) {
        _normal_request->cancel(NET_TIMEOUT);
        _normal_request.reset();
    }
    DEBUG("logid:%s controller timeout, retry_count:%u", _logid.c_str(), _retry_count);
    if (_user_callback) {
        _user_callback(shared_from_this(), _request, nullptr);
    }
    _self.reset();
    _cond.notify_all();
}

void Controller::on_rpc_failed(int error_code) {
    if (_status != RUNNING) {
        return;
    }

    _cost = _timer.tick();
    _status = FAILED;
    _error_code = error_code;
    _response = nullptr;
    cleanup();

    _backup_request.reset();
    _normal_request.reset();
    DEBUG("logid:%s controller failed, error_code:%d, retry_count:%u",
            _logid.c_str(), error_code, _retry_count);
    if (_user_callback) {
        _user_callback(shared_from_this(), _request, nullptr);
    }
    _self.reset();
    _cond.notify_all();
}

void Controller::on_rpc_canceled(bool run_callback) {
    if (_status != RUNNING) {
        return;
    }

    _cost = _timer.tick();
    _status = CANCELED;
    _error_code = NET_CANCELED;
    cleanup();

    _normal_request.reset();
    _backup_request.reset();

    // run user callback
    if (run_callback && _user_callback) {
        _user_callback(shared_from_this(), _request, nullptr);
    }
    _self.reset();
    // wake up user thread
    _cond.notify_all();
}

void Controller::controller_timeout_wrapper(ControllerWeakPtr controller) {
    ControllerPtr lock_controller = controller.lock();
    if (lock_controller) {
        lock_controller->handle_rpc_timeout();
    }
}

void Controller::backup_request_wrapper(ControllerWeakPtr controller) {
    ControllerPtr lock_controller = controller.lock();
    if (lock_controller) {
        lock_controller->handle_backup_request();
    }
}

void Controller::handle_rpc_timeout() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_status != RUNNING) {
        return;
    }
    _timeout_task_id = INVALID_TASK_ID;
    on_rpc_timeout();
}

void Controller::handle_backup_request() {
    std::lock_guard<std::mutex> lock(_mutex);
    DEBUG("start backup request...");
    _backup_request_task_id = INVALID_TASK_ID;
    if (_status == RUNNING) {
        if (check_need_retry()) {
            ++_retry_count;
            _backup_request.reset(new RequestController(this));
            int ret = _backup_request->issue_rpc();
            if (ret == NET_SUCC) {
                _has_backup_request = true;
            } else if (!normal_request_running()) {
                // should not happen
                on_rpc_failed(_backup_request->error_code());
            }
        }
    }
}

int Controller::join() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_status == INIT) {
        return NET_INVALID_ARGUMENT;
    } else if (_status != RUNNING && _status != SUBMITING) {
        return status_2_ret_code(_status);
    } else {
        ++_user_thread_joining;
        // rpc RUNNING, wait until rpc end
        while (_status == RUNNING || _status == SUBMITING) {
            // running tasks in user thread
            while (!_pending_tasks.empty()) {
                // copy task, to avoid clearing _pending_tasks by task itself
                TaskFunc task = _pending_tasks.front();
                _pending_tasks.pop_front();
                task();
            }
            if (_status == RUNNING || _status == SUBMITING) {
                _cond.wait_for(lock, Milliseconds(_options.total_timeout_ms));
            }
        }
        --_user_thread_joining;
    }
    return status_2_ret_code(_status);
}

int Controller::detach() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_status == INIT) {
        return NET_INVALID_ARGUMENT;
    } else if (_status != RUNNING && _status != SUBMITING) {
        return status_2_ret_code(_status);
    } else {
        if (!_self) {
            _self = shared_from_this();
        }
        return NET_SUCC;
    }
}

void Controller::cancel(bool run_callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_status == INIT || _status == SUBMITING) {
        _self.reset();
        cancel_bg_task(_submit_task_id);
        _error_code = NET_CANCELED;
        _status = CANCELED;
    } else if (_status == RUNNING) {
        on_rpc_canceled(run_callback);
    } else {
        // do nothing
    }
}

void Controller::cancel_pending_bg_tasks() {
    for (auto& id : _bg_tasks) {
        cancel_bg_task(id);
    }
    _bg_tasks.clear();
    _pending_tasks.clear();
}
 
ControllerAddresser::ControllerAddresser() : _cur_id(0) {}
 
ControllerAddresser::~ControllerAddresser() {}
 
ControllerAddresser& ControllerAddresser::get_instance() {
    static ControllerAddresser _instance;
    return _instance;
}

ControllerId ControllerAddresser::regist_internal(ControllerWeakPtr instance) {
    if (instance.expired()) {
        return INVALID_CONTROLLER_ID;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    ControllerId id = ++(_cur_id);
    _id_2_instance_map[id] = instance;
    return id;
}

void ControllerAddresser::remove_internal(ControllerId id) {
    std::lock_guard<std::mutex> lock(_mutex);
    _id_2_instance_map.erase(id);
}

ControllerWeakPtr ControllerAddresser::address_internal(ControllerId id) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _id_2_instance_map.find(id);
    return iter == _id_2_instance_map.end() ? ControllerWeakPtr() : iter->second;
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

