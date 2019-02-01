/**
 * @file connection.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:55:55
 * @brief 一个tcp物理连接
 *  
 **/
 
#ifndef WRPC_NETWORK_CONNECTION_H_
#define WRPC_NETWORK_CONNECTION_H_

#include <deque>
#include <mutex>

#include "common/end_point.h"
#include "interface/readable.h"
#include "interface/writable.h"
#include "network/network_common.h"
#include "utils/timer.h"

namespace wrpc {
 
class Connection : public Writable, public Readable {
private:
    EndPoint _end_point;
    int _sock_fd;

    Connection(const Connection&) = delete;
    Connection& operator = (const Connection&) = delete;

public:
    explicit Connection(const EndPoint& end_point);
    ~Connection();

    int connect(int32_t timeout_ms);

    int close();

    bool connected() const { return _sock_fd > 0; }

    const EndPoint& end_point() const { return _end_point; }

    virtual ssize_t read(char* buf, size_t size, int32_t timeout_ms);

    virtual ssize_t read_line(char* buf, size_t max_size, int32_t timeout_ms);

    virtual ssize_t write(const char* buf, size_t size, int32_t timeout_ms);

    int get_fd() const { return _sock_fd; }
};

typedef std::function<ConnectionPtr (int32_t)> ConnectionCreator;

class ConnectionPool {
private:
    std::deque<ConnectionPtr> _pool;
    mutable std::mutex _mutex;
    const size_t _max_size;
    ConnectionCreator _connection_creator;

    static const size_t DEFAULT_MAX_SIZE = 10;

    // disallow copy
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator = (const ConnectionPool&) = delete;
public:
    explicit ConnectionPool(const ConnectionCreator& connection_creator,
            size_t max_size = DEFAULT_MAX_SIZE)
            : _max_size(max_size),
              _connection_creator(connection_creator) {}
    explicit ConnectionPool(ConnectionCreator&& connection_creator,
            size_t max_size = DEFAULT_MAX_SIZE)
            : _max_size(max_size),
              _connection_creator(std::move(connection_creator)) {}
    ~ConnectionPool() {}

    // allow move constructor
    ConnectionPool(ConnectionPool&&) = default;

    // fetch a connection and hand over the ownership
    // create new connection by _connection_creator if pool is empty
    ConnectionPtr fetch(int32_t timeout_ms);

    // give back a connection and take back the ownership
    // release connection if pool is full
    void give_back(ConnectionPtr&& connection);

    size_t availability() const;

    size_t max_size() const { return _max_size; }
};

} // end namespace wrpc
 
#endif // WRPC_NETWORK_CONNECTION_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

