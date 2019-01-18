/**
 * @file load_balancer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 15:53:54
 * @brief 负载均衡策略接口，重试策略和负载均衡共用接口
 *        可对首次请求和重试请求指定不同的负载均衡策略
 **/
 
#ifndef WRPC_INTERFACE_LOAD_BALANCER_H_
#define WRPC_INTERFACE_LOAD_BALANCER_H_
 
#include <map>
#include <functional>
#include <vector>

#include "common/end_point.h"
#include "common/rpc_feedback.h"
#include "interface/end_point_watcher.h"
#include "utils/factory.h"
 
namespace wrpc {

struct LoadBalancerContext {
    std::string logid;
    uint32_t retry_count;
    size_t hash_code;
    // history try list
    std::vector<EndPoint> try_list;
    // store session data for lb strategy
    uint64_t data;
};

typedef std::pair<int, EndPoint> LoadBalancerOutput;
 
class LoadBalancer : public IEndPointWatcher {
public:
    explicit LoadBalancer(const std::string& name) : _name(name) {}
    virtual ~LoadBalancer() {}

public:
    virtual LoadBalancerOutput select(LoadBalancerContext& context) = 0;
    virtual void feedback(const FeedbackInfo& info) {
        // default implement
        // do not need feedback
    }

    // on end point changed
    virtual void on_remove_one(const EndPoint& endpoint) = 0;
    virtual void on_add_one(const EndPoint& endpoint) = 0;
    virtual void on_update_all(const EndPointList& endpoint_list) = 0;

    const std::string& name() const { return _name; }

protected:
    std::string _name;
};

typedef StrategyCreator<LoadBalancer, const std::string&> LBCreator;
typedef StrategyDeleter<LoadBalancer> LBDeleter;
typedef StrategyFactory<LoadBalancer, const std::string&> LBFactory;
typedef LBFactory::StrategyUniquePtr LoadBalancerPtr;

} // end namespace wrpc

#define REGISTER_LOAD_BALANCER(clazz, name) \
    REGISTER_STRATEGY_NAMELY(::wrpc::LoadBalancer, clazz, name, lb)
 
#endif // WRPC_INTERFACE_LOAD_BALANCER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

