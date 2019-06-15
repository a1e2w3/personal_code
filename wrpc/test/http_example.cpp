/**
 * @file http_example.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-11 21:39:11
 * @brief 
 *  
 **/
 
#include "wrpc/network/channel.h"
#include "wrpc/network/controller.h"
#include "wrpc/message/http_message.h"

#include "wrpc/test/mock_server.h"

using namespace wrpc;

static void callback(ControllerPtr controller, IRequest* req, IResponse* res) {
    fprintf(stdout, "run callback, error code:%d, cost:%ums\n",
            controller->error_code(), controller->total_cost());
    if (controller->is_canceled()) {
        fprintf(stdout, "rpc canceled\n");
    } else if (controller->is_failed()) {
        fprintf(stdout, "rpc failed\n");
    } else if (controller->is_timeout()) {
        fprintf(stdout, "rpc timeout\n");
    } else {
        HttpResponse* response = (HttpResponse *) (res);
        fprintf(stdout, "http response version[%u.%u] code[%d] reason[%s]\n", 
                response->version().first, response->version().second, 
                response->code(), response->reason().c_str());
                
        for (const auto& pair : response->headers()) {
            fprintf(stdout, "http response header [%s : %s]\n", 
                    pair.first.c_str(), pair.second.c_str());
        }

        if (response->is_chunked()) {
            for (const ChunkData& chunk : response->chunks()) {
                fprintf(stdout, "http response chunk size[%lu] ext[%s] data[%s]\n",
                        chunk.size(), chunk.extension().c_str(), chunk.data());
            }
        } else if (response->body_len() > 0) {
            fprintf(stdout, "http response body [%s]\n", response->body());
        }
    }
}

static void build_request(HttpRequest* request, const std::string& logid) {
    //request->set_method(HTTP_GET);
    request->set_host("www.baidu.com");
    request->set_content_type("application/json; charset=utf-8");
    request->set_uri("/http/test/v1");
    request->set_from("wrpc_test");
    request->set_header("logid", logid.c_str());
}

static void http_test() {
    ChannelPtr channel = Channel::make_channel();
    ChannelOptions options;
    options.protocol = "http_get";
    options.total_timeout_ms = 3000;
    options.connect_timeout_ms = 1000;
    options.backup_request_timeout_ms = 100;
    options.max_retry_num = 2;

    std::string logid = "f852bf7fa11544ce9fefa26eaf9ff093";
    //std::string address = "list://10.212.6.245:80";
    std::string address = "http://127.0.0.1:12345";

    int ret = channel->init(address, options);
    if (NET_SUCC != ret) {
        fprintf(stderr, "init channel failed with ret: %d.\n", ret);
        return;
    }

    //fprintf(stderr, "channel ref count: %ld.\n", channel.use_count());
    ControllerPtr controller = channel->create_controller();
    if (!controller) {
        fprintf(stderr, "create controller failed.\n");
        return;
    }
    //fprintf(stderr, "channel ref count: %ld.\n", channel.use_count());

    // make request
    HttpRequest* request = (HttpRequest *) (controller->get_request());
    build_request(request, logid);
    controller->set_logid(logid);
    controller->submit(&callback);
    controller->join();

    //fprintf(stderr, "channel ref count: %ld.\n", channel.use_count());
    //controller.reset();
    //fprintf(stderr, "channel ref count: %ld.\n", channel.use_count());
}
 
int main(int argc, char** argv) {
    // start mock server
    MockServer http_server(12345, HTTP);
    http_server.start();
    http_test();
    http_server.shut_down();
    return 0;
}
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

