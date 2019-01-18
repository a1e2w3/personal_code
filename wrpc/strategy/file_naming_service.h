/**
 * @file file_naming_service.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 11:14:50
 * @brief 文件静态配置的后端地址列表
 *  
 **/
 
#ifndef WRPC_STRATEGY_FILE_NAMING_SERVICE_H_
#define WRPC_STRATEGY_FILE_NAMING_SERVICE_H_
 
#include "interface/naming_service.h"
 
namespace wrpc {
 
class FileNamingService : public INamingService {
public:
    FileNamingService(const std::string& protocol) : INamingService(protocol) {}
    virtual ~FileNamingService() {}

    virtual int refresh(const std::string& address);
};
 
} // end namespace wrpc
 
#endif // WRPC_STRATEGY_FILE_NAMING_SERVICE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

