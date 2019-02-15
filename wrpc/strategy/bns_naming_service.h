/**
 * @file bns_naming_service.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 14:05:27
 * @brief 
 *  
 **/
 
#ifndef WRPC_STRATEGY_BNS_NAMING_SERVICE_H_
#define WRPC_STRATEGY_BNS_NAMING_SERVICE_H_
 
#include "interface/naming_service.h"
 
namespace wrpc {
 
class BNSNamingService : public INamingService {
public:
    explicit BNSNamingService(const std::string& protocol) : INamingService(protocol) {}
    virtual ~BNSNamingService() {}

    virtual int refresh(const std::string& address);
};
 
} // end namespace wrpc
 
#endif // WRPC_STRATEGY_BNS_NAMING_SERVICE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

