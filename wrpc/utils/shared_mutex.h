/**
 * @file shared_lock_guard.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 16:00:37
 * @brief ¶ÁÐ´Ëø
 *  
 **/
 
#ifndef WRPC_UTILS_SHARED_LOCK_GUARD_H_
#define WRPC_UTILS_SHARED_LOCK_GUARD_H_
 
#include <pthead.h>
 
namespace wrpc {

class SharedMutex {
public:
	SharedMutex() {
		pthread_rwlock_init(&_rw_lock, nullptr);
	}
	~SharedMutex() {
		pthread_rwlock_destroy(&_rw_lock);
	}

	int lock_shared() {
		return pthread_rwlock_rdlock(&_rw_lock);
	}

	void unlock_shared() {
		pthread_rwlock_unlock(&_rw_lock);
	}

	int lock() {
		return pthread_rwlock_wrlock(&_rw_lock);
	}

	void unlock() {
		pthread_rwlock_unlock(&_rw_lock);
	}

private:
	pthread_rwlock_t _rw_lock;
};
 
class SharedLockGuard {
public:
    explicit SharedLockGuard(SharedMutex& mutex) : _mutex(mutex) {
        _mutex.lock_shared();
    }
    ~SharedLockGuard() {
        _mutex.unlock_shared();
    }
private:
    SharedMutex& _mutex;
};

class ExclusiveLockGuard {
public:
    explicit ExclusiveLockGuard(std::shared_mutex& mutex) : _mutex(mutex) {
        _mutex.lock();
    }
    ~ExclusiveLockGuard() {
        _mutex.unlock();
    }
private:
	std::shared_mutex& _mutex;
};
 
} // end namespace wrpc
 
#endif // WRPC_UTILS_SHARED_LOCK_GUARD_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

