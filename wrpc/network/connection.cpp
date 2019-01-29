/**
 * @file connection.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:56:00
 * @brief 
 *  
 **/
 
#include "network/connection.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include "utils/common_define.h"
#include "utils/net_utils.h"
#include "utils/timer.h"
#include "utils/write_log.h"
 
namespace wrpc {
 
Connection::Connection(const EndPoint& end_point)
    : _end_point(end_point), _sock_fd(-1) {}

Connection::Connection(EndPoint&& end_point)
    : _end_point(std::move(end_point)), _sock_fd(-1) {}

Connection::~Connection() {
    close();
}

int Connection::connect(int32_t timeout_ms) {
    if (connected()) {
        DEBUG("Connection: connected already");
        return NET_SUCC;
    }

    if (!is_valid_port(_end_point.port())) {
        WARNING("Connection invalid port[%u]", _end_point.port());
        return NET_INVALID_ARGUMENT;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_end_point.port());
    int ret = inet_pton(AF_INET, _end_point.host().c_str(), &addr.sin_addr);
    if (1 != ret) {
        WARNING("Connection inet_pton failed: %d host: %s", ret, _end_point.host().c_str());
        return NET_INTERNAL_ERROR;
    }
    
    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
    	WARNING("Connection create socket failed: %d, errno: [%d:%s]", sock_fd, errno, strerror(errno));
        return NET_INTERNAL_ERROR;
    }

    ret = connect_with_timeout(sock_fd, (const struct sockaddr*)(&addr), timeout_ms);
    if (ret != NET_SUCC) {
        close_connection(sock_fd);
        WARNING("Connection connect failed: %d address:[%s]", ret, _end_point.to_string().c_str());
        return ret;
    }
    _sock_fd = sock_fd;
    DEBUG("connect fd[%d] to [%s] success", _sock_fd, _end_point.to_string().c_str());
    return NET_SUCC;
}

int Connection::close() {
    int ret = close_connection(_sock_fd);
    _sock_fd = -1;
    return ret;
}

ssize_t Connection::read(char* buf, size_t size, int32_t timeout_ms) {
    return readn(_sock_fd, buf, size, timeout_ms);
}

ssize_t Connection::read_line(char* buf, size_t max_size, int32_t timeout_ms) {
    return readline(_sock_fd, buf, max_size, timeout_ms);
}

ssize_t Connection::write(const char* buf, size_t size, int32_t timeout_ms) {
    return writen(_sock_fd, buf, size, timeout_ms);
}

ConnectionPtr ConnectionPool::fetch(int32_t timeout_ms) {
	std::lock_guard<std::mutex> lock(_mutex);
    if (_pool.empty()) {
        return _connection_creator(timeout_ms);
    } else {
        ConnectionPtr connection = std::move(_pool.front());
        _pool.pop_front();
        return connection;
    }
}

void ConnectionPool::give_back(ConnectionPtr&& connection) {
	if (connection) {
	    std::lock_guard<std::mutex> lock(_mutex);
        if (_pool.size() < _max_size) {
            _pool.push_back(std::move(connection));
        } else {
            connection.reset();
        }
	}
}

size_t ConnectionPool::availability() const {
    std::lock_guard<std::mutex> lock(_mutex);
    size_t size = _pool.size();
    return size;
}

} // namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

