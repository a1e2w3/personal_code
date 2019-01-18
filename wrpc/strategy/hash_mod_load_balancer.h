/**
 * @file hash_mod_load_balancer.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-13 13:36:22
 * @brief ���ڹ�ϣȡģ�ĸ��ؾ���, �����Դ�����������
 *        �ڽڵ�����Ƶ��(��listЭ��)�������, �ﵽ��hash������Ч�ʽϸ�
 *        �ڵ���Ƶ���������, ������һ����hash
 *  
 **/
 
#ifndef WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
#define WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
 
#include <mutex>
#include <vector>
#include <unordered_set>

#include "interface/load_balancer.h"

namespace wrpc {

class HashModLoadBalancer : public LoadBalancer {
public:
    explicit HashModLoadBalancer(const std::string& name) : LoadBalancer(name) {}
    virtual ~HashModLoadBalancer() {}

public:
    virtual LoadBalancerOutput select(LoadBalancerContext& context);

    // on end point changed
    virtual void on_set_death(const EndPoint& endpoint);
    virtual void on_set_alive(const EndPoint& endpoint);
    virtual void on_remove_one(const EndPoint& endpoint);
    virtual void on_add_one(const EndPoint& endpoint);
    virtual void on_update_all(const EndPointList& endpoint_list);

private:
    std::mutex _mutex;

    std::vector<EndPoint> _candicates;
    std::unordered_set<EndPoint> _death;
};

} // end namespace wrpc
 
#endif // WRPC_STRATEGY_HASH_MOD_LOAD_BALANCER_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

