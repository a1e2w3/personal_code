/**
 * @file http_naming_service.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 15:17:42
 * @brief http”Ú√˚Ω‚Œˆ
 *  
 **/
 
#ifndef WRPC_STRATEGY_HTTP_NAMING_SERVICE_H_
#define WRPC_STRATEGY_HTTP_NAMING_SERVICE_H_
 
#include "interface/naming_service.h"
 
namespace wrpc {
 
class HttpNamingService : public INamingService {
public:
    explicit HttpNamingService(const std::string& protocol) : INamingService(protocol) {}
    virtual ~HttpNamingService() {}

    virtual int refresh(const std::string& address);
};
 
} // end namespace wrpc
 
#endif // WRPC_STRATEGY_HTTP_NAMING_SERVICE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

