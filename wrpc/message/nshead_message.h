/**
 * @file nshead_message.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 15:29:50
 * @brief nshead–≠“È
 *  
 **/
 
#ifndef WRPC_MESSAGE_NEHEAD_MESSAGE_H_
#define WRPC_MESSAGE_NEHEAD_MESSAGE_H_
 
#include <memory.h>
#include <vector>

#include "nshead.h"

#include "interface/message.h"
 
namespace wrpc {

class NsheadMessage : public IMessage {
public:
    NsheadMessage() { memset(&_header, 0, sizeof(nshead_t)); }
    explicit NsheadMessage(size_t capacity) : _buffer(capacity) {  memset(&_header, 0, sizeof(nshead_t)); }
    ~NsheadMessage() {}

    nshead_t& header() { return _header; }
    const nshead_t& header() const { return _header; }

    void clear() { return _buffer.clear(); }
    size_t capacity() const { return _buffer.capacity(); };
    size_t size() const { return _buffer.size(); };
    void reserve(size_t capacity) { _buffer.reserve(capacity); }

    void append(const char* buf, size_t len);

    virtual int write_to(Writable* writable, int32_t timeout);

    virtual int read_from(Readable* readable, int32_t timeout);

private:
    char* buffer() { return &*(_buffer.begin()); }

private:
    typedef std::vector<char> AutoBuffer;

    nshead_t _header;
    AutoBuffer _buffer;
};

typedef NsheadMessage NsheadRequest;
typedef NsheadMessage NsheadResponse;

} // end namespace wrpc
 
#endif // WRPC_MESSAGE_NEHEAD_MESSAGE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

