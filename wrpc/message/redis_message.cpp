/***************************************************************************
 * 
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file redis_message.cpp
 * @author wangcong07(wangcong07@baidu.com)
 * @date 2018-02-12 17:55:46
 * @brief 
 *  
 **/
 
#include "message/redis_message.h"

#include "utils/common_define.h"
#include "utils/timer.h"
#include "utils/write_log.h"

namespace wrpc {

int RedisRequest::write_to(Writable* writable, int32_t timeout) {
    if (writable == nullptr || _params.empty()) {
        return NET_INVALID_ARGUMENT;
    }
    std::ostringstream oss;
    oss << '*' << _params.size() << "\r\n";
    for (size_t i = 0; i < _params.size(); ++i) {
        oss << '$' << _params[i].length() << "\r\n" << _params[i] << "\r\n";
    }

    std::string message = oss.str();
    DEBUG("redis request: [%s]", message.c_str());

    const char* buf = &(message[0]);
    ssize_t ret = writable->write(buf, message.length(), timeout);
    if (ret != message.length()) {
        return ret < 0 ? ret : NET_SEND_FAIL;
    }
    return NET_SUCC;
}

int RedisResponse::read_from(Readable* readable, int32_t timeout) {
    if (readable == nullptr) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;
    _list.clear();

    // read first line
    ssize_t ret = readable->read_line(_buffer, sizeof(_buffer) - 1, remain);
    if (ret <= 3) {
        return ret < 0 ? ret : NET_PARSE_MESSAGE_FAIL;
    }

    if (timer.timeout(&remain)) {
        return NET_TIMEOUT;
    }
    _buffer[ret - 2] = '\0';
    _line_len = ret - 2;

    if (_buffer[0] == '*') {
        unsigned int res_cnt = 0;
        if (1 != sscanf(_buffer, "*%u\r\n", &res_cnt)) {
            WARNING("redis response bulk length line[%s] illegal", _buffer);
            return NET_PARSE_MESSAGE_FAIL;
        }

        DEBUG("redis response line[%s] bulk count[%d]", _buffer, res_cnt);
        for (unsigned int i = 0; i < res_cnt; ++i) {
            ret = readable->read_line(_buffer, sizeof(_buffer) - 1, remain);
            if (ret <= 3) {
                return ret < 0 ? ret : NET_PARSE_MESSAGE_FAIL;
            }

            if (timer.timeout(&remain)) {
                return NET_TIMEOUT;
            }

            _buffer[ret - 2] = '\0';
            _line_len = ret - 2;

            int err = read_one_item(readable, remain);
            if (err != NET_SUCC) {
                return err;
            }
        }
        return NET_SUCC;
    } else {
        return read_one_item(readable, remain);
    }
}

int RedisResponse::read_one_item(Readable* readable, int32_t timeout_ms) {
    switch (_buffer[0]) {
    case '+':
        return read_status_res();
    case '-':
        return read_error_res();
    case ':':
        return read_integer_res();
    case '$':
        return read_bulk_res(readable, timeout_ms);
    default:
        return NET_MESSAGE_NOT_MATCH;
    }
}

int RedisResponse::read_status_res() {
    _list.emplace_back(STATUS, 0);
    RedisReponseItem& item = _list.back();
    item._message = std::string(_buffer + 1);
    DEBUG("redis response line[%s] status[%s]", _buffer, item.message().c_str());
    return NET_SUCC;
}

int RedisResponse::read_error_res() {
    char* p_sep = strchr(_buffer + 1, ' ');
    _list.emplace_back(ERROR, 0);
    RedisReponseItem& item = _list.back();
    if (p_sep == nullptr) {
        item._message = std::string(_buffer + 1);
    } else {
        *p_sep = '\0';
        item._message = std::string(_buffer + 1);
        item._detail = std::string(p_sep + 1);
    }
    DEBUG("redis response line[%s] error[%s] detail[%s]",
            _buffer, item.message().c_str(), item.detail().c_str());
    return NET_SUCC;
}

int RedisResponse::read_integer_res() {
    long int_value = 0;
    try {
        int_value = std::stol(_buffer + 1);
    } catch(...) {
         WARNING("redis response interger line[%s] illegal", _buffer);
         return NET_PARSE_MESSAGE_FAIL;
    }

    _list.emplace_back(INTERGER, _line_len - 1);
    RedisReponseItem& item = _list.back();
    item._int_data = int_value;
    strncpy(item._data.get(), _buffer + 1, _line_len - 1);
    DEBUG("redis response line[%s] integer[%ld]", _buffer, item.integer());
    return NET_SUCC;
}

int RedisResponse::read_bulk_res(Readable* readable, int32_t timeout) {
    int bulk_len = 0;
    if (1 != sscanf(_buffer, "$%d\r\n", &bulk_len)) {
        WARNING("redis response bulk length line[%s] illegal", _buffer);
        return NET_PARSE_MESSAGE_FAIL;
    }

    if (bulk_len < 0) {
        DEBUG("redis response line[%s] nil", _buffer);
        _list.emplace_back(NIL, 0);
        return NET_SUCC;
    }

    _list.emplace_back(BULK, bulk_len);
    RedisReponseItem& item = _list.back();
    ssize_t ret = readable->read_line(item._data.get(), bulk_len + 2, timeout);
    if (ret != bulk_len + 2) {
        return ret < 0 ? ret : NET_RECV_FAIL;
    }
    item._data[bulk_len] = '\0';
    DEBUG("redis response line[%s] bulk len[%lu]", _buffer, item.data_len());
    return NET_SUCC;
}

REGISTER_REQUEST(RedisRequest, redis)
REGISTER_RESPONSE(RedisResponse, redis)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

