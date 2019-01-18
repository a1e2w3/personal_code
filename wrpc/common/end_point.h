/**
 * @file end_point.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 12:58:33
 * @brief 
 *  
 **/
 
#ifndef WRPC_COMMON_END_POINT_H_
#define WRPC_COMMON_END_POINT_H_
 
#include <functional>
#include <stdint.h>
#include <string>
#include <unordered_set>

namespace wrpc {

typedef uint16_t port_t;

class EndPoint;
typedef std::unordered_set<EndPoint> EndPointList;
 
// cannot change once created
class EndPoint {
private:
    std::string _host;          // should be ip
    port_t _port;
    std::string _end_point_str; // host:port
    mutable size_t _hash_code;

public:
    EndPoint() : _port(0), _hash_code(0) {}
    EndPoint(const std::string& host, port_t port);
    EndPoint(std::string&& host, port_t port);
    ~EndPoint();

    // allow copy
    EndPoint(const EndPoint&);
    EndPoint& operator = (const EndPoint&);
    // allow move
    EndPoint(EndPoint&&);
    EndPoint& operator = (EndPoint&&);

    const std::string& host() const { return _host; }

    port_t port() const { return _port; }

    const std::string& to_string() const { return _end_point_str; }

    size_t hash_code() const;
};

// relational operators
// make EndPoint can be the key of std::map and std::unordered_map
inline bool operator == (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.hash_code() == ep2.hash_code()) {
        return ep1.to_string() == ep2.to_string();
    } else {
        return false;
    }
}
inline bool operator != (const EndPoint& ep1, const EndPoint& ep2) {
    if (ep1.hash_code() == ep2.hash_code()) {
        return ep1.to_string() != ep2.to_string();
    } else {
        return true;
    }
}
inline bool operator < (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.to_string() < ep2.to_string();
}
inline bool operator <= (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.to_string() <= ep2.to_string();
}
inline bool operator > (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.to_string() > ep2.to_string();
}
inline bool operator >= (const EndPoint& ep1, const EndPoint& ep2) {
    return ep1.to_string() >= ep2.to_string();
}

} // end namespace wrpc

namespace std {
// specialization std::hash for wrpc::EndPoint
// make EndPoint can be the key of unordered_map
template<>
struct hash<::wrpc::EndPoint> {
public:
    size_t operator () (const ::wrpc::EndPoint& ep) const {
        return ep.hash_code();
    }
};
} // end namespace std
 
#endif // WRPC_COMMON_END_POINT_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

