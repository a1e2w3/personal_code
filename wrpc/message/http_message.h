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
    void set_uri(const std::string& uri) { _uri = uri; }
    const std::string& get_host() const { return _host; }
    void set_host(const std::string& host) { _host = host; }

    const HttpHeaders& headers() const { return _headers; }
    void set_header(const std::string& key, const std::string& value);
    const std::string& get_header(const std::string& key) const;
    bool has_header(const std::string& key) const;

    // specific header
    void set_content_type(const std::string& content_type) {
        set_header("Content-Type", content_type);
    }
    void set_user_agent(const std::string& user_agent) {
        set_header("User-Agent", user_agent);
    }
    void set_cache_control(const std::string& cache_control) {
        set_header("Cache-Control", cache_control);
    }
    void set_referer(const std::string& referer) {
        set_header("Referer", referer);
    }
    void set_accept(const std::string& accept) {
        set_header("Accept", accept);
    }
    void set_accept_charset(const std::string& accept_charset) {
        set_header("Accept-Charset", accept_charset);
    }
    void set_accept_encoding(const std::string& accept_encoding) {
        set_header("Accept-Encoding", accept_encoding);
    }
    void set_accept_language(const std::string& accept_language) {
        set_header("Accept-Language", accept_language);
    }
    void set_authorization(const std::string& authorization) {
        set_header("Authorization", authorization);
    }
    void set_from(const std::string& from) {
        set_header("From", from);
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

