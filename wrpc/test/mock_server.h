/**
 * @file http_example.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2019-02-01 14:53:51
 * @brief mock server for test
 *
 **/

#pragma once

#include <atomic>
#include <thread>

#include "wrpc/utils/common_define.h"
 
namespace wrpc {

enum Protocol {
    HTTP = 0,
    REDIS = 1,
};
 
class MockServer {
public:
    MockServer(port_t listen_port, Protocol protocol)
        : _listen_port(listen_port), _protocol(protocol) {}

    ~MockServer() { shut_down(); }

    int start();
    void shut_down();

private:
    void bg_thread_func();

    void do_response(int sock_fd);
 
private:
    const port_t _listen_port;
    int _listen_fd = -1;
    const Protocol _protocol;
    bool _running = false;
    std::atomic<bool> _stop;

    std::thread _bg_thread;
};
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

