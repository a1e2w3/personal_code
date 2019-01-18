/**
 * @file end_point.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:58:36
 * @brief 
 *  
 **/
 
#include "common/end_point.h"
 
#include <sstream>

namespace wrpc {

static std::string end_point_to_string(const std::string& host, port_t port) {
    std::ostringstream oss;
    oss << host << ":" << port;
    return oss.str();
}
 
EndPoint::EndPoint(const std::string& host, port_t port)
    : _host(host), _port(port),
      _end_point_str(end_point_to_string(_host, _port)), _hash_code(0) {}

EndPoint::EndPoint(std::string&& host, port_t port)
    : _host(std::move(host)), _port(port),
      _end_point_str(end_point_to_string(_host, _port)), _hash_code(0) {}

EndPoint::EndPoint(const EndPoint& other)
    : _host(other._host), _port(other._port),
      _end_point_str(other._end_point_str), _hash_code(other._hash_code) {}

EndPoint::EndPoint(EndPoint&& other)
    : _host(std::move(other._host)),
      _port(other._port),
      _end_point_str(std::move(other._end_point_str)),
      _hash_code(other._hash_code) {
    other._port = 0;
    other._hash_code = 0;
}

EndPoint::~EndPoint() {}

EndPoint& EndPoint::operator = (const EndPoint& other) {
    if (this != &other) {
        _host = other._host;
        _port = other._port;
        _end_point_str = other._end_point_str;
        _hash_code = other._hash_code;
    }
    return *this;
}

EndPoint& EndPoint::operator = (EndPoint&& other) {
    if (this != &other) {
        _host = std::move(other._host);
        _port = other._port;
        _end_point_str = std::move(other._end_point_str);
        _hash_code = other._hash_code;

        other._port = 0;
        other._hash_code = 0;
    }
    return *this;
}

size_t EndPoint::hash_code() const {
    if (_hash_code == 0) {
        std::hash<std::string> string_hasher;
        _hash_code = string_hasher(to_string());
    }
    return _hash_code;
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

