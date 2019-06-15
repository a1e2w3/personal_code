// Copyright 2019 Baidu.com, Inc. All Rights Reserved
// Author: wangcong07(wangcong07@baidu.com)
// File: mock_server.cpp
// Date: 2019-02-01 14:53:45
// 
//

#include "wrpc/test/mock_server.h"

#include <errno.h>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "wrpc/utils/net_utils.h"
#include "wrpc/utils/write_log.h"
 
namespace wrpc {

const size_t MAX_REQ_SIZE = 1024 * 1024 * 10; // 10MB
const int32_t READ_TIMEOUT_MS = 500; // 500ms
const int32_t WRITE_TIMEOUT_MS = 500; // 500ms
 
int MockServer::start() {
    if (!is_valid_port(_listen_port)) {
        WARNING("invalid listen port: [%u]", _listen_port);
        return NET_INVALID_ARGUMENT;
    }
    if (_running) {
        WARNING("mock server is running already");
        return NET_INVALID_ARGUMENT;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd <= 0) {
        WARNING("create socket error: %s(errno: %d)", strerror(errno), errno);
        return NET_INTERNAL_ERROR;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(_listen_port);

    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        WARNING("bind socket error: %s(errno: %d)", strerror(errno), errno);
        close(listenfd);
        return NET_INTERNAL_ERROR;
    }

    // accept timeout
    struct timeval timeout = {0, 100000}; // 100ms
    if (setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) != 0) {
        WARNING("set accept timeout failed");
        close(listenfd);
        return NET_INTERNAL_ERROR;
    }

    if (listen(listenfd, 10) < -1) {
        WARNING("listen socket error: %s(errno: %d)", strerror(errno), errno);
        close(listenfd);
        return 0;
    }

    _stop.store(false);
    _listen_fd = listenfd;
    NOTICE("mock server listen port: [%u]", _listen_port);

    std::thread bg_thread(std::bind(&MockServer::bg_thread_func, this));
    _bg_thread.swap(bg_thread);
    _running = true;

    return NET_SUCC;
}

void MockServer::shut_down() {
    if (_running) {
        _stop.store(true);
        close(_listen_fd);
        _listen_fd = -1;
        _bg_thread.join();
        _running = false;
        NOTICE("mock server shut down");
    }
}

static ssize_t response_data(int sock_fd, const char* data, size_t len) {
    ssize_t ret = writen(sock_fd, data, len, WRITE_TIMEOUT_MS);
    DEBUG("response msg to client: %ld", ret);
    return ret;
}

static void do_response_http(int sock_fd) {
    const char* response =
            "HTTP/1.1 204 NoData\r\n"
            "\r\n\r\n";
    response_data(sock_fd, response, strlen(response));
}

static void do_response_redis(int sock_fd) {
    const char* response = "$-1\r\n";
    response_data(sock_fd, response, strlen(response));
}
 
void MockServer::do_response(int sock_fd) {
    switch (_protocol) {
    case HTTP:
        do_response_http(sock_fd);
        break;
    case REDIS:
        do_response_redis(sock_fd);
        break;
    default:
        WARNING("unknow protocol: %d", _protocol);
        break;
    }
}

void MockServer::bg_thread_func() {
    NOTICE("mock server bg thread start");
    std::unique_ptr<char[]> request_buffer(new char[MAX_REQ_SIZE]);
    while(!_stop.load()) {
        int connfd = accept(_listen_fd, (struct sockaddr*)nullptr, nullptr);
        if (connfd < 0) {
            if (errno != EAGAIN) {
                WARNING("accept socket error: %s(errno: %d)", strerror(errno), errno);
            }
            continue;
        }
        DEBUG("accept a connection");
        // do not read anything
        //ssize_t n = readn(connfd, request_buffer.get(), MAX_REQ_SIZE, READ_TIMEOUT_MS);
        //DEBUG("recv msg from client: %ld", n);
        do_response(connfd);
        // do not close connfd
    }
    NOTICE("mock server bg thread stopped");
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

