/**
 * @file list_naming_service.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:50
 * @brief list://ip:port,ip:port,ip:port
 **/
 
#ifndef WRPC_STRATEGY_LIST_NAMING_SERVICE_H_
#define WRPC_STRATEGY_LIST_NAMING_SERVICE_H_
 
#include "interface/naming_service.h"
 
namespace wrpc {
 
class ListNamingService : public INamingService {
public:
    explicit ListNamingService(const std::string& protocol) : INamingService(protocol) {}
    virtual ~ListNamingService() {}

    virtual int refresh(const std::string& address);
private:
    std::string _last_address;
};
 
} // end namespace wrpc
 
#endif // WRPC_STRATEGY_LIST_NAMING_SERVICE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

