/**
 * @file message.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 13:56:08
 * @brief 协议数据格式抽象
 *  
 **/
 
#ifndef WRPC_INTERFACE_MESSAGE_H_
#define WRPC_INTERFACE_MESSAGE_H_
 
#include <functional>
#include <map>

#include "interface/readable.h"
#include "interface/writable.h"
#include "utils/factory.h"

namespace wrpc {
 
class IRequest {
public:
    IRequest() : _hash_code(std::hash<IRequest*>()(this)) {}
    virtual ~IRequest() {}

    virtual int write_to(Writable* writable, int32_t timeout) = 0;

    void set_hash_code(size_t hash_code) { _hash_code = hash_code; }
    size_t hash_code() const { return _hash_code; }

protected:
    size_t _hash_code;
};

typedef StrategyCreator<IRequest> RequestCreator;
typedef StrategyDeleter<IRequest> RequestDeleter;
typedef StrategyFactory<IRequest> RequestFactory;

class IResponse {
public:
    IResponse() {}
    virtual ~IResponse() {}

    virtual int read_from(Readable* readable, int32_t timeout) = 0;
};

typedef StrategyCreator<IResponse> ResponseCreator;
typedef StrategyDeleter<IResponse> ResponseDeleter;
typedef StrategyFactory<IResponse> ResponseFactory;

class IMessage : public IRequest, public IResponse {};

} // end namespace wrpc
 
#define REGISTER_REQUEST(clazz, name) \
        REGISTER_STRATEGY(::wrpc::IRequest, clazz, name, req)

#define REGISTER_RESPONSE(clazz, name) \
        REGISTER_STRATEGY(::wrpc::IResponse, clazz, name, res)

#endif // WRPC_INTERFACE_MESSAGE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

