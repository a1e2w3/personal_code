/**
 * @file nshead_message.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 15:29:46
 * @brief 
 *  
 **/
 
#include "message/nshead_message.h"

#include "utils/common_define.h"
#include "utils/timer.h"
#include "utils/write_log.h"
 
namespace wrpc {

void NsheadMessage::append(const char* buf, size_t len) {
    if (len == 0) {
        return;
    }

    reserve(size() + len);
    _buffer.insert(_buffer.end(), buf, buf + len);
}

int NsheadMessage::write_to(Writable* writable, int32_t timeout) {
    if (writable == nullptr || _buffer.empty()) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;
    size_t body_len = _buffer.size();
    _header.body_len = body_len;
    _header.magic_num = NSHEAD_MAGICNUM;
    DEBUG("nshead body len: %lu magic_num: %lx", _header.body_len, _header.magic_num);

    ssize_t ret = writable->write(
            reinterpret_cast<const char*>(&_header), sizeof(nshead_t), remain);
    if (ret != sizeof(nshead_t)) {
        return ret < 0 ? ret : NET_SEND_FAIL;
    }

    if (timer.timeout(&remain)) {
        return NET_TIMEOUT;
    }

    ret = writable->write(buffer(), body_len, remain);
    if (ret != body_len) {
        return ret < 0 ? ret : NET_SEND_FAIL;
    }
    return NET_SUCC;
}

int NsheadMessage::read_from(Readable* readable, int32_t timeout) {
    if (readable == nullptr) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;
    ssize_t ret = readable->read(
            reinterpret_cast<char*>(&_header), sizeof(nshead_t), remain);
    if (ret != sizeof(nshead_t)) {
        return ret < 0 ? ret : NET_RECV_FAIL;
    }
    DEBUG("nshead body len: %lu", _header.body_len);

    if (_header.magic_num != NSHEAD_MAGICNUM) {
        WARNING("nshead magic_num not match: [%lx vs %lx]", _header.magic_num, NSHEAD_MAGICNUM);
        return NET_MESSAGE_NOT_MATCH;
    }

    if (timer.timeout(&remain)) {
        return NET_TIMEOUT;
    }

    clear();
    reserve(_header.body_len);
    ret = readable->read(buffer(), _header.body_len, remain);
    if (ret != _header.body_len) {
        return ret < 0 ? ret : NET_RECV_FAIL;
    }
    return NET_SUCC;
}
 
REGISTER_REQUEST(NsheadRequest, nshead)
REGISTER_RESPONSE(NsheadResponse, nshead)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

