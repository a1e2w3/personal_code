/**
 * @file writable.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 13:58:55
 * @brief 
 *  
 **/
 
#ifndef WRPC_INTERFACE_WRITABLE_H_
#define WRPC_INTERFACE_WRITABLE_H_
 
#include <stdint.h>
 
namespace wrpc {
 
class Writable {
public:
	Writable() {}
	virtual ~Writable() {}

    virtual ssize_t write(const char* buf, size_t size, int32_t timeout_ms) = 0;
};
 
} // end namespace wrpc
 
#endif // WRPC_INTERFACE_WRITABLE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

