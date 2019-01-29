/**
 * @file controller.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:55:43
 * @brief һ��rpc�Ự�������ܰ���������������ԣ�backup request��
 *  
 **/
 
#ifndef WRPC_NETWORK_CONTROLLER_H_
#define WRPC_NETWORK_CONTROLLER_H_
 
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "common/options.h"
#include "common/rpc_feedback.h"
#include "interface/load_balancer.h"
#include "interface/message.h"
#include "network/network_common.h"
#include "utils/background.h"
#include "utils/common_define.h"
#include "utils/timer.h"
 
namespace wrpc {

typedef std::function<void (ControllerPtr, IRequest*, IResponse*)> RPCCallback;

static const ControllerId INVALID_CONTROLLER_ID = 0;

class EventDispatcher;

enum ControllerStatus {
    CANCELED = -3,
    TIMEOUT = -2,
    FAILED = -1,
    INIT = 0,
    SUBMITING = 1,
    RUNNING = 2,
    SUCCESS = 3,
};

/*
 * һ�κ������
 */
class RequestController {
private:
    friend class Controller;
    friend class std::default_delete<RequestController>;

    explicit RequestController(Controller*);
    ~RequestController();
    // disallow copy and move
    RequestController(const RequestController&) = delete;
    RequestController(RequestController&&) = delete;
    RequestController& operator = (const RequestController&) = delete;
    RequestController& operator = (RequestController&&) = delete;

public:
    int get_fd() const;

    int error_code() const {
        return _error_code;
    }

    const FeedbackInfo& get_feedback() const {
        return _feedback_info;
    }

    bool is_running() const {
        return _is_running;
    }

    bool is_success() const {
        return error_code() == NET_SUCC;
    }

    IResponse* get_response() { return _response; }
    int issue_rpc();
    int retry(int ret_code);
    void cancel(int code);
    int on_epoll_in();
    int on_epoll_error();
    void feedback(int code);

private:
    int listen();
    int unlisten();

private:
    bool _is_running;
    Controller* _controller;
    ConnectionPtr _connection;
    IResponse* _response;
    int _error_code;
    uint32_t _try_count;
    FeedbackInfo _feedback_info;
    MicrosecondsTimer _timer;
};

class Controller : public std::enable_shared_from_this<Controller> {
private:
    // should only create and destroy by Channel
    Controller(ChannelPtr&& channel, const RPCOptions& options);
    ~Controller();
    // disallow copy and move
    Controller(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator = (const Controller&) = delete;
    Controller& operator = (Controller&&) = delete;

    friend class Channel;
    friend class EventDispatcher;
    friend class RequestController;
public:
    void set_logid(const std::string& logid) {
        _logid = logid;
        _lb_context.logid = logid;
    }

    /**
     * @brief �ύrpc, ����ֱ�����󱻳ɹ����������Դ�������
     *
     * @return int
     *        NET_SUCC: �����ͳɹ�, ����joinͬ���ȴ�����, ��detach�������ں�̨�������
     *        <0: ����ʧ��, ������
     */
    int submit();

    /**
     * @brief �ύrpc��ע��ص�, ����ֱ�����󱻳ɹ����������Դ�������
     *
     * @return int
     *        NET_SUCC: �����ͳɹ�, ����joinͬ���ȴ�����, ��detach�������ں�̨�������
     *        <0: ����ʧ��, ���ش�����, ��ʱ�ص������ᱻ����
     */
    int submit(const RPCCallback& callback);

    /**
     * @brief �ύrpc, ������������, �����ں�̨����
     *
     * @return int
     *        NET_SUCC: ��̨����������, ����joinͬ���ȴ�����, ��detach�������ں�̨�������
     *        <0: �ύʧ��, ������
     */
    int submit_async();

    /**
     * @brief �ύrpc��ע��ص�, ������������, �����ں�̨����
     *
     * @return int
     *        NET_SUCC: ��̨����������, ����joinͬ���ȴ�����, ��detach�������ں�̨�������
     *        <0: �ύʧ��, ������, ��ʱ�û��ص����ᱻ����
     */
    int submit_async(const RPCCallback& callback);

    /**
     * @brief ͬ���ȴ�ֱ��rpc���, ��ʱ��ʧ��
     *        �����Ѿ��ύ��rpc, join����ʱ�û��ص�һ����ִ�����
     *        ������running״̬, join��������
     *
     * @return int
     *         NET_SUCC: rpc ��������
     *         <0: rpc��ʱ��ʧ��, ������
     */
    int join();

    /**
     * @brief ��rpc�ں�̨�����������, ���н�����, ���۳ɹ����, ����ִ���û��ص�
     *        detach�ɹ����û����ͷų��е�controllerָ��, ��rpc���ᱻȡ��
     *        ������running״̬, detach��������
     *
     * @return int
     *         NET_SUCC: �ɹ�
     *         <0: ʧ��, ������
     */
    int detach();

    /**
     * @brief ȡ�������е�rpc, ��rpc���ڽ�����(δ�ύ/���/��ʱ/ʧ��), ��ֱ�ӷ���
     *
     * @param [in] run_callback : bool
     *        �Ƿ���Ҫִ���û��ص�
     *
     * @return void
     */
    void cancel(bool run_callback = true);

    /// ��ȡ�������Ӧ, ��Ӧֻ��rpc������ɵ�����²ŷ��ط�nullptr
    IRequest* get_request() { return _request; }
    IResponse* get_response() { return _response; }

    /// ��ȡrpc��ǰ״̬
    ControllerStatus status() const { return _status; }
    bool is_submiting() const { return _status == SUBMITING; }
    bool is_running() const { return _status == RUNNING; }
    bool is_success() const { return _status == SUCCESS; }
    bool is_canceled() const { return _status == CANCELED; }
    bool is_failed() const { return _status == FAILED; }
    bool is_timeout() const { return _status == TIMEOUT; }

public:
    // options getter & setter
    void set_total_timeout(int32_t timeout_ms) {
        _options.total_timeout_ms = timeout_ms;
    }
    int32_t get_total_timeout() const {
        return _options.total_timeout_ms;
    }
    void set_connect_timeout(int32_t timeout_ms) {
        _options.connect_timeout_ms = timeout_ms;
    }
    int32_t get_connect_timeout() const {
        return _options.connect_timeout_ms;
    }
    void set_backup_repuest_timeout(int32_t timeout_ms) {
        _options.backup_request_timeout_ms = timeout_ms;
    }
    int32_t get_backup_repuest_timeout() const {
        return _options.backup_request_timeout_ms;
    }
    void set_max_retry(uint32_t max_retry) {
        _options.max_retry_num = max_retry;
    }
    uint32_t get_max_retry() const {
        return _options.max_retry_num;
    }
    uint32_t get_retry_count() const {
        return _retry_count;
    }
    bool has_backup_request() const {
        return _has_backup_request;
    }

    // debug end point
    void set_debug_endpoint(const EndPoint& debug_ep) {
        _use_debug_ep = true;
        _debug_ep = debug_ep;
    }

    const std::string& get_protocol() const;

    int error_code() const { return _error_code; }

    uint32_t total_cost() const { return _cost; }

private:
    // back ground task wrapper
    static void submit_wrapper(ControllerWeakPtr controller, const RPCCallback& callback);
    static void epoll_in_wrapper(ControllerWeakPtr controller, int fd);
    static void epoll_error_wrapper(ControllerWeakPtr controller, int fd);
    static void controller_timeout_wrapper(ControllerWeakPtr controller);
    static void backup_request_wrapper(ControllerWeakPtr controller);

    void on_epoll_in(int fd);
    void on_epoll_error(int fd);
    void handle_epoll_in(int fd);
    void handle_epoll_error(int fd);
    void feedback(const FeedbackInfo& info);

    int fetch_connection(ConnectionPtr&);
    void giveback_connection(ConnectionPtr&&, bool close);

    bool check_need_retry() {
        return _retry_count < _options.max_retry_num && !_timer.timeout();
    }
    void handle_rpc_timeout();
    void handle_backup_request();
    void cleanup();

    // ״̬���仯
    void on_rpc_success();
    void on_rpc_timeout();
    void on_rpc_failed(int error_code);
    void on_rpc_canceled(bool run_callback = true);

    bool normal_request_running() const {
        return _normal_request && _normal_request->is_running();
    }
    bool backup_request_running() const {
        return _backup_request && _backup_request->is_running();
    }
    void cancel_pending_bg_tasks();

private:
    ControllerId _id;
    RPCOptions _options;
    std::string _logid;
    MillisecondsCountdownTimer _timer;
    ChannelPtr _channel;
    IRequest* _request;
    IResponse* _response;
    RPCCallback _user_callback;
    LoadBalancerContext _lb_context;
    int _error_code;
    uint32_t _cost;

    // debug end point
    bool _use_debug_ep;
    EndPoint _debug_ep;

    // rpc context
    ControllerStatus _status;
    uint32_t _retry_count;
    bool _has_backup_request;
    std::unique_ptr<RequestController> _normal_request;
    std::unique_ptr<RequestController> _backup_request;

    // back ground task
    BackgroundTaskId _submit_task_id;
    BackgroundTaskId _timeout_task_id;
    BackgroundTaskId _backup_request_task_id;

    uint32_t _user_thread_joining;
    std::mutex _mutex;  // mutex
    std::condition_variable _cond; // cond
    typedef std::function<void()> TaskFunc;
    std::deque<TaskFunc> _pending_tasks;
    std::deque<BackgroundTaskId> _bg_tasks;

    // detachģʽ��, ��������, ��ֹController������
    ControllerPtr _self;
};

// Address controller by controller_id
// singleton
class ControllerAddresser {
private:
    ControllerAddresser();

    static ControllerAddresser& get_instance();

public:
    ~ControllerAddresser();

    static ControllerId regist(ControllerWeakPtr instance) {
        return get_instance().regist_internal(instance);
    }
    static void remove(ControllerId id) {
        get_instance().remove_internal(id);
    }
    static ControllerWeakPtr address(ControllerId id) {
        return get_instance().address_internal(id);
    }

private:
    ControllerId regist_internal(ControllerWeakPtr);
    void remove_internal(ControllerId id);
    ControllerWeakPtr address_internal(ControllerId id);

private:
    std::mutex _mutex;
    ControllerId _cur_id;
    std::unordered_map<ControllerId, ControllerWeakPtr> _id_2_instance_map;
};
 
} // end namespace wrpc
 
#endif // WRPC_NETWORK_CONTROLLER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

