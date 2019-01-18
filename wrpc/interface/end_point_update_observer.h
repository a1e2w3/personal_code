/**
 * @file end_point_updater.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 16:31:29
 * @brief 
 *     后台定期刷新end_point列表的接口
 *  
 **/
 
#ifndef WRPC_INTERFACE_END_POINT_UPDATE_OBSERVER_H_
#define WRPC_INTERFACE_END_POINT_UPDATE_OBSERVER_H_
 
#include <set>
#include <mutex>

#include "common/end_point.h"
#include "utils/common_define.h"
 
namespace wrpc {
 
class EndPointUpdateObserver {
public:
	EndPointUpdateObserver() {}
	virtual ~EndPointUpdateObserver() {}

	virtual void on_update(const EndPointList& endpoint_list) = 0;
};

class EndPointUpdateObserverable {
public:
	EndPointUpdateObserverable() {}
	virtual ~EndPointUpdateObserverable() { _observers.clear(); }

    bool add_observer(EndPointUpdateObserver* observer) {
    	if (observer != nullptr) {
    		std::lock_guard<std::mutex> lock(_mutex);
    	    auto ret = _observers.insert(observer);
    	    return ret.second;
    	}
        return false;
    }
    bool remove_observer(EndPointUpdateObserver* observer) {
        if (observer != nullptr) {
        	std::lock_guard<std::mutex> lock(_mutex);
    	    return _observers.erase(observer) > 0;
    	}
        return false;
    }

protected:
    void notify_update(const EndPointList& endpoint_list) {
    	std::lock_guard<std::mutex> lock(_mutex);
        for (auto& observer : _observers) {
            observer->on_update(endpoint_list);
        }
    }

private:
    std::mutex _mutex;
    std::set<EndPointUpdateObserver*> _observers;
};
 
} // end namespace wrpc
 
#endif // WRPC_INTERFACE_END_POINT_UPDATE_OBSERVER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

