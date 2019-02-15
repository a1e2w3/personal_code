/**
 * @file naming_service.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 16:57:55
 * @brief 
 *      名字服务，定期通过naming刷新下游endpoint列表，通知各个订阅方
 **/
 
#ifndef WRPC_INTERFACE_NAMING_SERVICE_H_
#define WRPC_INTERFACE_NAMING_SERVICE_H_
 
#include "interface/end_point_update_observer.h"
#include "utils/factory.h"
 
namespace wrpc {
 
class INamingService : public EndPointUpdateObserverable {
public:
    explicit INamingService(const std::string& protocol) : _protocol(protocol) {}
    virtual ~INamingService() {}

    virtual int refresh(const std::string& address) = 0;

    const std::string& protocol() const { return _protocol; }

protected:
    std::string _protocol;
};

typedef StrategyCreator<INamingService, const std::string&> NamingServiceCreator;
typedef StrategyDeleter<INamingService> NamingServiceDeleter;
typedef StrategyFactory<INamingService, const std::string&> NamingServiceFactory;
typedef NamingServiceFactory::StrategyUniquePtr NamingServicePtr;
 
} // end namespace wrpc

#define REGISTER_NAMING_SERVICE(clazz, name) \
    REGISTER_STRATEGY_NAMELY(::wrpc::INamingService, clazz, name, epu)
 
#endif // WRPC_INTERFACE_NAMING_SERVICE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

