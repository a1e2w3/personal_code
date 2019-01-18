/**
 * @file http_message.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-11 11:56:39
 * @brief 
 *  
 **/
 
#include "message/http_message.h"

#include <stdio.h>

#include "utils/common_define.h"
#include "utils/timer.h"
#include "utils/write_log.h"
 
namespace wrpc {

static const char* const CRLF = "\r\n";

static const char* const _g_method_table[] = {
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
};

static const char* method_2_str(HTTP_METHOD method) {
    return _g_method_table[method];
}

// default as HTTP/1.1
static const HttpVersion DEFAULT_HTTP_VERSION(1, 1);
 
HttpRequest::HttpRequest(HTTP_METHOD method)
    : _method(method),
      _version(DEFAULT_HTTP_VERSION),
      _uri("/") {}
 
HttpRequest::~HttpRequest() {}

void HttpRequest::pre_process() {
    if (!has_header("Transfer-Encoding")) {
        // TODO do not set content length if is chunked
        set_header("Content-Length", std::to_string(body_len()));
    }
}

int HttpRequest::write_to(Writable* writable, int32_t timeout) {
    if (writable == nullptr) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;

    // pre process
    pre_process();

    // serialize request header
    char* header_cursor = _header_buf;
    size_t header_buf_remain = sizeof(_header_buf);
    size_t header_size = 0;
    int ret = snprintf(header_cursor, header_buf_remain, "%s %s HTTP/%u.%u%sHost: %s%s",
            method_2_str(_method),
            _uri.c_str(),
            _version.first, _version.second,
            CRLF,
            _host.c_str(),
            CRLF);
    if (ret < 0 || ret >= header_buf_remain) {
        // exceed buffer size
        return NET_PARSE_MESSAGE_FAIL;
    }
    header_cursor += ret;
    header_buf_remain -= ret;
    header_size += ret;

    for (const auto& head_pair : _headers) {
        ret = snprintf(header_cursor, header_buf_remain, "%s: %s%s",
                head_pair.first.c_str(), head_pair.second.c_str(), CRLF);
        if (ret < 0 || ret >= header_buf_remain) {
            // exceed buffer size
            return NET_PARSE_MESSAGE_FAIL;
        }
        header_cursor += ret;
        header_buf_remain -= ret;
        header_size += ret;
    }

    // append CRLF
    ret = snprintf(header_cursor, header_buf_remain, "%s", CRLF);
    if (ret < 0 || ret >= header_buf_remain) {
        // exceed buffer size
        return NET_PARSE_MESSAGE_FAIL;
    }
    header_size += ret;

    if (timer.timeout(&remain)) {
        return NET_TIMEOUT;
    }

    DEBUG("http request header: [%s]", _header_buf);
    ret = writable->write(_header_buf, header_size, remain);
    if (ret != header_size) {
        return NET_SEND_FAIL;
    }

    if (!_body.empty()) {
        if (timer.timeout(&remain)) {
            return NET_TIMEOUT;
        }

        // write body
        size_t body_length = body_len();
        DEBUG("http request body size: %lu", body_length);
        ret = writable->write(body(), body_length, remain);
        if (ret != body_length) {
            return NET_SEND_FAIL;
        }
    }

    return NET_SUCC;
}

void HttpRequest::set_header(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

const std::string& HttpRequest::get_header(const std::string& key) const {
    static std::string NOT_FOUND_HEADER_VALUE("");
    auto iter = _headers.find(key);
    return iter == _headers.end() ? NOT_FOUND_HEADER_VALUE : iter->second;
}

bool HttpRequest::has_header(const std::string& key) const {
    return _headers.find(key) != _headers.end();
}
 
void HttpRequest::append_body(const char* buf, size_t len) {
    if (len == 0) {
        return;
    }

    _body.reserve(_body.size() + len);
    _body.insert(_body.end(), buf, buf + len);
}

HttpResponse::HttpResponse()
    : _code(200),
      _version(DEFAULT_HTTP_VERSION),
      _is_chunked(false) {
    _body.push_back('\0');
}

HttpResponse::~HttpResponse() {}

const std::string& HttpResponse::get_header(const std::string& key) const {
    static std::string NOT_FOUND_HEADER_VALUE("");
    auto iter = _headers.find(key);
    return iter == _headers.end() ? NOT_FOUND_HEADER_VALUE : iter->second;
}

bool HttpResponse::has_header(const std::string& key) const {
    return _headers.find(key) != _headers.end();
}

int HttpResponse::read_from(Readable* readable, int32_t timeout) {
    if (readable == nullptr) {
        return NET_INVALID_ARGUMENT;
    }
    
    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;

    // reset
    _headers.clear();
    _body.clear();
    _body.push_back('\0');
    _chunks.clear();
    _is_chunked = false;
    
    ssize_t ret = readable->read_line(_header_buf, sizeof(_header_buf) - 1, remain);
    if (ret < 0) {
        return ret;
    }
    _header_buf[ret] = '\0';
    
    int err = sscanf(_header_buf, "HTTP/%u.%u %u %511s\r\n",
            &(_version.first), &(_version.second), &_code, _ext_buf);
    if (err != 4) {
        WARNING("parse http repsponse status line [%s] failed.", _header_buf);
        return NET_MESSAGE_NOT_MATCH;
    }
    
    _reason = _ext_buf;
    DEBUG("http response status line: [HTTP/%u.%u %03u %s]", _version.first, _version.second, _code, _reason.c_str());
    
    if (timer.timeout(&remain)) {
        return NET_TIMEOUT;
    }
    
    err = read_header(readable, remain);
    if (err != NET_SUCC) {
        return err;
    }
    if (has_header("Content-Length")) {
        return read_normal_body(readable, remain);
    } else if (get_header("Transfer-Encoding") == "chunked") {
        _is_chunked = true;
        return read_chunked_body(readable, remain);
    }
    return NET_SUCC;
}

int HttpResponse::read_header(Readable* readable, int32_t timeout) {
    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;

    // read headers
    while (true) {
        ssize_t ret = readable->read_line(_header_buf, sizeof(_header_buf) - 1, remain);
        if (ret < 0) {
            return NET_RECV_FAIL;
        } else if (ret <= 2) {
            if (timer.timeout(&remain)) {
                return NET_TIMEOUT;
            }
            break;
        }
        _header_buf[ret - 2] = '\0';

        char* p_sep = strchr(_header_buf, ':');
        if (p_sep == nullptr) {
            WARNING("parse http response header [%s] failed. ignore", _header_buf);
            continue;
        }
        *p_sep = '\0';
        // trim
        for (char *p = p_sep; p > _header_buf; --p) {
            if (*p != ' ' && *p != '\t') {
                break;
            }
            *p = '\0';
        }
        ++p_sep;
        while (*p_sep == ' ' || *p_sep == '\t') {
            ++p_sep;
        }
        _headers.emplace(_header_buf, p_sep);
        DEBUG("http response header: [%s : %s]", _header_buf, p_sep);

        if (timer.timeout(&remain)) {
            return NET_TIMEOUT;
        }
    }
    return NET_SUCC;
}

int HttpResponse::read_normal_body(Readable* readable, int32_t timeout) {
    size_t body_len = 0;
    auto iter = _headers.find("Content-Length");
    if (iter != _headers.end()) {
        try {
            body_len = std::stoull(iter->second);
        } catch (...) {
            body_len = 0;
            WARNING("invalid http response content-type [%s]", iter->second.c_str());
            return NET_PARSE_MESSAGE_FAIL;
        }
    }

    if (body_len > 0) {
        _body.reserve(body_len + 1);
        ssize_t ret = readable->read(&*(_body.begin()), body_len, timeout);
        _body[body_len] = '\0';
        if (ret != body_len) {
            WARNING("read http response body length %lu failed, ret:%d.", body_len, ret);
            return ret < 0 ? ret : NET_RECV_FAIL;
        }
    }
    return NET_SUCC;
}

int HttpResponse::read_chunked_body(Readable* readable, int32_t timeout) {
    MillisecondsCountdownTimer timer(timeout);
    MillisecondsCountdownTimer::rep_type remain = timeout;

    int ret = read_one_chunk(readable, remain);
    while (ret > 0) {
        if (timer.timeout(&remain)) {
            return NET_TIMEOUT;
        }
        ret = read_one_chunk(readable, remain);
    }

    return ret;
}

int HttpResponse::read_one_chunk(Readable* readable, int32_t timeout) {
    // read chunk header
    ssize_t ret = readable->read_line(_header_buf, sizeof(_header_buf) - 1, timeout);
    if (ret < 0) {
        return NET_RECV_FAIL;
    }
    _header_buf[ret] = '\0';

    unsigned long chunk_size = 0;
    _ext_buf[0] = '\0';
    int err = sscanf(_header_buf, "%lx%511s\r\n", &chunk_size, _ext_buf);
    if (err < 1) {
        WARNING("parse chunk header [%s] failed:%d", _header_buf, err);
        return NET_MESSAGE_NOT_MATCH;
    }

    _chunks.emplace_back(chunk_size);
    ChunkData& chunk = _chunks.back();
    chunk._chunk_extension = _ext_buf;
    ret = readable->read(chunk._chunk_data.get(), chunk_size + 2, timeout);
    if (ret != chunk_size + 2) {
        WARNING("read one chunk failed. expect size[%lu] ret[%l]", chunk_size, ret);
        return ret < 0 ? ret : NET_RECV_FAIL;
    }
    chunk._chunk_data[chunk_size] = '\0';

    DEBUG("read one chunk, size[%lu] extension[%s]", chunk_size, chunk._chunk_extension.c_str());
    return chunk_size;
}

REGISTER_REQUEST(HttpRequest, http)
REGISTER_RESPONSE(HttpResponse, http)

bool _s_http_get_request_registered = RequestFactory::get_instance().register_creator("http_get", create_http_request<HTTP_GET>);
REGISTER_RESPONSE(HttpResponse, http_get)

bool _s_http_post_request_registered = RequestFactory::get_instance().register_creator("http_post", create_http_request<HTTP_POST>);
REGISTER_RESPONSE(HttpResponse, http_post)
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

