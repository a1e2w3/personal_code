/**
 * @file redis_message.h
 * @author wangcong07(wangcong07@baidu.com)
 * @date 2018-02-12 17:55:43
 * @brief redis–≠“È
 *  
 **/
 
#ifndef WRPC_MESSAGE_REDIS_MESSAGE_H_
#define WRPC_MESSAGE_REDIS_MESSAGE_H_
 
#include <deque>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "interface/message.h"
#include "utils/string_utils.h"

namespace wrpc {

class RedisRequest : public IRequest {
public:
    RedisRequest() {}
    virtual ~RedisRequest() {}

    template<class... Args>
    void set_command(const std::string& command, Args&&... args) {
        _params.clear();
        _params.reserve(sizeof...(Args) + 1);
        append_param(command, std::forward<Args>(args)...);
    }

    template<class... Args>
    void set_command(std::string&& command, Args&&... args) {
        _params.clear();
        _params.reserve(sizeof...(Args) + 1);
        append_param(std::move(command), std::forward<Args>(args)...);
    }

    virtual int write_to(Writable* writable, int32_t timeout);

private:
    template<class T, class... Args>
    inline void append_param(T&& param, Args&&... args) {
        _params.emplace_back(to_string(std::forward<T>(param)));
        append_param(std::forward<Args>(args)...);
    }

    // partial specialization
    template<class T>
    inline void append_param(T&& param) {
        _params.emplace_back(to_string(std::forward<T>(param)));
    }

private:
    std::vector<std::string> _params;
};

enum RedisResponseType {
    UNKNOW = -2,
    ERROR = -1,
    STATUS = 0,
    INTERGER = 1,
    BULK = 2,
    NIL = 3,
};

class RedisResponse;

class RedisReponseItem {
public:
    explicit RedisReponseItem(RedisResponseType type = UNKNOW, size_t data_len = 0)
        : _type(type),
          _int_data(0),
          _data_len(data_len),
          _data(data_len == 0 ? nullptr : new char[_data_len + 2]) {}
    ~RedisReponseItem() {}

    // getter
    RedisResponseType type() const { return _type; }
    long integer() const { return _int_data; }
    const std::string& message() const { return _message; }
    const std::string& detail() const { return _detail; }
    size_t data_len() const { return _data_len; }
    const char* data() const { return _data.get(); }

private:
    friend class RedisResponse;

    RedisResponseType _type;
    long _int_data;
    std::string _message;
    std::string _detail;
    size_t _data_len;
    std::unique_ptr<char[]> _data;
};

typedef std::deque<RedisReponseItem> RedisReponseList;

class RedisResponse : public IResponse {
public:
    RedisResponse() {}
    virtual ~RedisResponse() {}

    virtual int read_from(Readable* readable, int32_t timeout);

    const RedisReponseList& response_list() const { return _list; }

private:
    int read_one_item(Readable* readable, int32_t timeout);

    int read_status_res();
    int read_error_res();
    int read_integer_res();
    int read_bulk_res(Readable* readable, int32_t timeout);

private:
    char _buffer[1024];
    size_t _line_len;
    RedisReponseList _list;
};
 
} // end namespace wrpc
 
#endif // WRPC_MESSAGE_REDIS_MESSAGE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

