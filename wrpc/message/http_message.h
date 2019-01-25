/**
 * @file http_message.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-11 11:56:36
 * @brief http–≠“È
 *  
 **/
 
#ifndef WRPC_MESSAGE_HTTP_MESSAGE_H_
#define WRPC_MESSAGE_HTTP_MESSAGE_H_

#include <deque>
#include <functional>
#include <map>
#include <memory> 
#include <vector>

#include "interface/message.h"
#include "utils/common_flags.h"

// TODO compression support
namespace wrpc {

enum HTTP_METHOD {
    HTTP_OPTIONS = 0,
    HTTP_GET = 1,
    HTTP_HEAD = 2,
    HTTP_POST = 3,
    HTTP_PUT = 4,
    HTTP_DELETE = 5,
    HTTP_TRACE = 6,
    HTTP_CONNECT = 7,
};

typedef std::pair<unsigned int, unsigned int> HttpVersion;
typedef std::map<std::string, std::string> HttpHeaders;

class HttpRequest : public IRequest {
public:
    explicit HttpRequest(HTTP_METHOD method = HTTP_GET);
    ~HttpRequest();

    virtual int write_to(Writable* writable, int32_t timeout);

    HTTP_METHOD get_method() const { return _method; }
    void set_method(HTTP_METHOD method) { _method = method; }
    const HttpVersion& get_version() const { return _version; }
    void set_version(unsigned int version, unsigned int sub_version) {
        _version.first = version;
        _version.second = sub_version;
    }
    const std::string& get_uri() const { return _uri; }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_uri(StringType&& uri) { _uri = std::forward<StringType>(uri); }
    const std::string& get_host() const { return _host; }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_host(StringType&& host) { _host = std::forward<StringType>(host); }

    const HttpHeaders& headers() const { return _headers; }
    template<typename Key, typename Value>
    typename std::enable_if<std::is_convertible<Key&&, std::string>::value && std::is_convertible<Value&&, std::string>::value, void>::type
    set_header(Key&& key, Value&& value) {
        _headers[std::forward<Key>(key)] = std::forward<Value>(value);
    }
    const std::string& get_header(const std::string& key) const;
    bool has_header(const std::string& key) const;

    // specific header
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_content_type(StringType&& content_type) {
        set_header("Content-Type", std::forward<StringType>(content_type));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_user_agent(StringType&& user_agent) {
        set_header("User-Agent", std::forward<StringType>(user_agent));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_cache_control(StringType&& cache_control) {
        set_header("Cache-Control", std::forward<StringType>(cache_control));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_referer(StringType&& referer) {
        set_header("Referer", std::forward<StringType>(referer));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_accept(StringType&& accept) {
        set_header("Accept", std::forward<StringType>(accept));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_accept_charset(StringType&& accept_charset) {
        set_header("Accept-Charset", std::forward<StringType>(accept_charset));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_accept_encoding(StringType&& accept_encoding) {
        set_header("Accept-Encoding", std::forward<StringType>(accept_encoding));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_accept_language(StringType&& accept_language) {
        set_header("Accept-Language", std::forward<StringType>(accept_language));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_authorization(StringType&& authorization) {
        set_header("Authorization", std::forward<StringType>(authorization));
    }
    template<typename StringType>
    typename std::enable_if<std::is_convertible<StringType&&, std::string>::value, void>::type
    set_from(StringType&& from) {
        set_header("From", std::forward<StringType>(from));
    }

    void append_body(const char* buf, size_t len);
    const char* body() const { return &*(_body.begin()); }
    char* body() { return &*(_body.begin()); }
    size_t body_len() const { return _body.size(); }

private:
    void pre_process();

private:
    HTTP_METHOD _method;
    HttpVersion _version;
    std::string _uri;
    std::string _host;
    HttpHeaders _headers;

    typedef std::vector<char> AutoBuffer;
    AutoBuffer _body;

    char _header_buf[WRPC_HTTP_MAX_HEADER_SIZE];
};

template<HTTP_METHOD METHOD>
IRequest* create_http_request() {
    return new (std::nothrow) HttpRequest(METHOD);
}

class HttpResponse;

class ChunkData {
public:
    ChunkData() : _chunk_size(0) {}
    explicit ChunkData(size_t size)
        : _chunk_size(size),
          _chunk_data(new char[_chunk_size + 2]) {
        _chunk_data[0] = '\0';
    }
    ~ChunkData() {}

    // getter
    size_t size() const { return _chunk_size; }
    const std::string& extension() const { return _chunk_extension; }
    const char* data() const { return _chunk_data.get(); }

private:
    friend class HttpResponse;
    size_t _chunk_size;
    std::string _chunk_extension;
    std::unique_ptr<char[]> _chunk_data;
};

typedef std::deque<ChunkData> ChunkList;
//typedef std::function<void (const ChunkData&)> OnChunkData;

class HttpResponse : public IResponse {
public:
    HttpResponse();
    virtual ~HttpResponse();

    virtual int read_from(Readable* readable, int32_t timeout);

    //void set_chunk_callback(const OnChunkData& callback) {
    //    _chunk_callback = callback;
    //}

    unsigned int code() const { return _code; }
    const std::string& reason() const { return _reason; }
    const HttpVersion& version() const { return _version; }
    
    const HttpHeaders& headers() const { return _headers; }
    const std::string& get_header(const std::string& key) const;
    bool has_header(const std::string& key) const;
    
    bool is_chunked() const { return _is_chunked; }

    size_t body_len() const { return _body.size(); }
    const char* body() const { return &*(_body.begin()); }

    const ChunkList& chunks() const { return _chunks; }

private:
    int read_header(Readable* readable, int32_t timeout);
    int read_normal_body(Readable* readable, int32_t timeout);
    int read_chunked_body(Readable* readable, int32_t timeout);

    int read_one_chunk(Readable* readable, int32_t timeout);

private:
    unsigned int _code;
    std::string _reason;
    HttpVersion _version;
    HttpHeaders _headers;
    bool _is_chunked;

    // response body if is not chunked
    typedef std::vector<char> AutoBuffer;
    AutoBuffer _body;
    
    //OnChunkData _chunk_callback;
    ChunkList _chunks;

    char _header_buf[WRPC_HTTP_MAX_HEADER_LINE_LEN];
    char _ext_buf[512]; // max 512 bytes
};

} // end namespace wrpc
 
#endif // WRPC_MESSAGE_HTTP_MESSAGE_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

