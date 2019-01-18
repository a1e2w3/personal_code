/**
 * @file readable.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-31 13:59:01
 * @brief 
 *  
 **/
 
#ifndef WRPC_INTERFACE_READABLE_H_
#define WRPC_INTERFACE_READABLE_H_
 
#include <stdint.h>
 
namespace wrpc {
 
class Readable {
public:
    Readable() {}
    virtual ~Readable() {}

    virtual ssize_t read(char* buf, size_t size, int32_t timeout_ms) = 0;
    
    virtual ssize_t read_line(char* buf, size_t max_size, int32_t timeout_ms) {
        // default implementation that read char one by one
        for (size_t i = 0; i < max_size; ++i) {
            ssize_t ret = read(buf + i, 1, timeout_ms);
            if (ret == 0) {
                // eof
                return i;
            } else if (ret < 0) {
                // read failed
                return ret;
            } else if (ret == 1 && buf[i] == '\n') {
                return i + 1;
            }
        }
        return max_size;
    }
};
 
} // end namespace wrpc
 
#endif // WRPC_INTERFACE_READABLE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

