/**
 * @file end_point_watcher.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 15:46:04
 * @brief 
 *      下游end point状态变化通知的接口
 **/
 
#ifndef WRPC_INTERFACE_END_POINT_WATCHER_H_
#define WRPC_INTERFACE_END_POINT_WATCHER_H_
 
#include <functional>
#include <set>
#include <mutex>

#include "common/end_point.h"
 
namespace wrpc {

// Endpoint列表状态变化的观察者
// 比如LoadBalancer
class IEndPointWatcher {
public:
	IEndPointWatcher() {}
	virtual ~IEndPointWatcher() {}

	virtual void on_set_death(const EndPoint& endpoint) {
	    // default implement
		on_remove_one(endpoint);
	}
	virtual void on_set_alive(const EndPoint& endpoint) {
	    // default implement
	    on_add_one(endpoint);
	}
	virtual void on_remove_one(const EndPoint& endpoint) = 0;
	virtual void on_add_one(const EndPoint& endpoint) = 0;
	virtual void on_update_all(const EndPointList& endpoint_list) = 0;
};

// Endpoint列表状态变化的通知者
// 比如EndPointManager
class IEndPointNotifier {
public:
    IEndPointNotifier() {}
    virtual ~IEndPointNotifier() { _watchers.clear(); }

public:
    bool add_watcher(IEndPointWatcher* watcher) {
    	if (watcher != nullptr) {
    	    std::lock_guard<std::mutex> lock(_mutex);
    	    auto ret = _watchers.insert(watcher);
    	    return ret.second;
    	}
        return false;
    }
    bool remove_watcher(IEndPointWatcher* watcher) {
    	if (watcher != nullptr) {
    		std::lock_guard<std::mutex> lock(_mutex);
    	    return _watchers.erase(watcher) > 0;
    	}
        return false;
    }

private:
    typedef std::function<void (IEndPointWatcher*)> NotifyFunc;
    void notify(const NotifyFunc& func) {
    	std::lock_guard<std::mutex> lock(_mutex);
    	for (auto& watcher : _watchers) {
    		func(watcher);
    	}
    }

protected:
    void notify_set_death(const EndPoint& endpoint) {
    	notify(std::bind(&IEndPointWatcher::on_set_death, std::placeholders::_1, endpoint));
    }
    void notify_set_alive(const EndPoint& endpoint) {
        notify(std::bind(&IEndPointWatcher::on_set_alive, std::placeholders::_1, endpoint));
    }
    void notify_remove_one(const EndPoint& endpoint) {
    	notify(std::bind(&IEndPointWatcher::on_remove_one, std::placeholders::_1, endpoint));
    }
    void notify_add_one(const EndPoint& endpoint) {
    	notify(std::bind(&IEndPointWatcher::on_add_one, std::placeholders::_1, endpoint));
    }
    void notify_update_all(const EndPointList& endpoint_list) {
    	notify(std::bind(&IEndPointWatcher::on_update_all, std::placeholders::_1, endpoint_list));
    }

private:
    std::mutex _mutex;
    std::set<IEndPointWatcher*> _watchers;
};
 
} // end namespace wrpc
 
#endif // WRPC_INTERFACE_END_POINT_WATCHER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

