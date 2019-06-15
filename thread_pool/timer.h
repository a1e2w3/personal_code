/**
 * @file timer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2017-08-25 17:27:55
 * @brief 计时工具
 *
 **/

#pragma once
 
#include <chrono>
 
namespace common {

typedef std::chrono::microseconds Microseconds;
typedef std::chrono::milliseconds Milliseconds;
typedef std::chrono::seconds Seconds;
typedef std::chrono::time_point<std::chrono::system_clock, Microseconds> TimePoint;

template<typename Duration>
inline typename Duration::rep get_timestamp() {
    auto tp = std::chrono::system_clock::now();
    return std::chrono::duration_cast<Duration>(tp.time_since_epoch()).count();
}

inline int64_t get_micro() {
    return get_timestamp<Microseconds>();
} 
 
inline int64_t get_milli() {
    return get_timestamp<Milliseconds>();
} 
 
inline int64_t get_second() {
    return get_timestamp<Seconds>();
}

template<typename Duration>
class Timer {
public:
    typedef Duration duration_type;
    typedef typename Duration::rep rep_type;

    Timer() : _start(get_timestamp<Duration>()) {}

    rep_type start_time() const {
        return _start;
    }

    rep_type now() const {
        return get_timestamp<Duration>();
    }

    rep_type tick() const {
        return now() - _start;
    }

    void reset() {
        _start = now();
    }

private:
    // disallow copy
    Timer(const Timer&) = delete;
    Timer& operator = (const Timer&) = delete;

    rep_type _start;
};

typedef Timer<Microseconds> MicrosecondsTimer;
typedef Timer<Milliseconds> MillisecondsTimer;
typedef Timer<Seconds> SecondsTimer;

template<typename Rep, typename Duration = Microseconds>
class ScopedTimer {
public:
    typedef Rep rep_type;
    typedef Duration duration_type;

    explicit ScopedTimer(Rep& rep) : _rep(rep), _timer() {}
    ~ScopedTimer() {
        _rep = static_cast<Rep>(_timer.tick());
    }

private:
    Rep& _rep;
    Timer<Duration> _timer;

    // disallow copy
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator = (const ScopedTimer&) = delete;
};

template<typename Duration>
class CountdownTimer {
public:
    typedef Duration duration_type;
    typedef typename Duration::rep rep_type;

    explicit CountdownTimer(rep_type timeout)
        : _timeout(timeout),
          _start(now()),
          _target(_start + _timeout) {}

    rep_type target_time() const {
        return _target;
    }

    rep_type start_time() const {
        return _start;
    }

    rep_type now() const {
        return get_timestamp<Duration>();
    }

    rep_type tick() const {
        return now() - _start;
    }

    rep_type remain() const {
        return _timeout < 0 ? -1 : _target - now();
    }

    bool timeout(rep_type* remain_time = nullptr) const {
        if (_timeout < 0) {
            if (remain_time) {
                *remain_time = -1;
            }
            return false;
        } else {
            rep_type re = remain();
            if (remain_time) {
                *remain_time = re;
            }
            return re <= 0;
        }
    }

    void reset(rep_type timeout) {
        _timeout = timeout;
        _start = now();
        _target = _start + _timeout;
    }

private:
    // disallow copy
    CountdownTimer(const CountdownTimer&) = delete;
    CountdownTimer& operator = (const CountdownTimer&) = delete;

    rep_type _timeout;
    rep_type _start;
    rep_type _target;
};

typedef CountdownTimer<Microseconds> MicrosecondsCountdownTimer;
typedef CountdownTimer<Milliseconds> MillisecondsCountdownTimer;
typedef CountdownTimer<Seconds> SecondsCountdownTimer;

} // end namespace common

// Usage:
// uint64_t cost = 0;
// TIMER(cost) {
//     do_something();
//     if (condition) break;
//     do_something();
// }
#define TIMER(val) \
    for (int __i = 0; __i < 1; ++__i) \
        for (::common::ScopedTimer<decltype(val)> t(val); __i < 1; ++__i)
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

