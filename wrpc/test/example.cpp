/**
 * @file example.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 14:50:10
 * @brief 
 *  
 **/
 
#include <assert.h>
#include <thread>

#include "network/channel.h"
#include "network/controller.h"
#include "message/nshead_mcpack_message.h"
#include "message/redis_message.h"
#include "utils/timer.h"

#include "test/mock_server.h"

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
    } else if (controller->get_protocol() == "redis") {
        RedisResponse* response = (RedisResponse*) (res);
        for (const RedisReponseItem& item : response->response_list()) {
            fprintf(stdout, "redis response [%d] [%ld] [%s] [%s] [%lu] [%s]\n",
                    item.type(), item.integer(), item.message().c_str(), item.detail().c_str(),
                    item.data_len(), item.data() == nullptr ? "" : item.data());
        }
    }
}

static void redis_test(const std::string& logid, const std::string& address) {
    ChannelPtr channel = Channel::make_channel();
    ChannelOptions options;
    options.protocol = "redis";
    options.total_timeout_ms = 3000;
    options.connect_timeout_ms = 100;
    options.backup_request_timeout_ms = 100;
    options.max_retry_num = 2;

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
    RedisRequest* request = (RedisRequest *) (controller->get_request());
    build_request(request, logid);
    controller->set_logid(logid);
    ret = controller->submit_async(&callback);
    fprintf(stderr, "submit controller ret: %d.\n", ret);
    assert(ret == NET_SUCC);

    // try submit twice
    ret = controller->submit_async(&callback);
    assert(ret == NET_INVALID_ARGUMENT);

    // running in back ground
    ret = controller->detach();
    fprintf(stderr, "detach controller ret: %d.\n", ret);

    //controller.reset();
    fprintf(stderr, "release controller ptr.\n");
}
 
int main(int argc, char** argv) {
    // start mock server
    MockServer redis_server(12345, REDIS);
    redis_server.start();

    std::string address = "list://127.0.0.1:12345";
    std::string logid = "f852bf7fa11544ce9fefa26eaf9ff093";
    //if (argc > 1) {
    //    address = argv[1];
    //    fprintf(stderr, "address: %s.\n", address.c_str());
    //}
    redis_test(logid, address);

    // wait rpc in back ground
    std::this_thread::sleep_for(Seconds(1));
    redis_server.shut_down();
    return 0;
}
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

