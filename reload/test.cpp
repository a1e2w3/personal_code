/**
 * @file test.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-24 11:18:03
 * @brief 
 *  
 **/
 
#include <assert.h>
#include <iostream>
#include <thread>

#include "reloadable.h"
#include "timer.h"

using namespace common;

class Resource;
static size_t g_resource_count = 0;
static Reloadable<Resource> g_resource;
static bool reload_switch = true;

class Resource {
public:
    Resource() : _id(++g_resource_count) {
        std::cout << "Init Resource: " << _id << std::endl;
    }

    ~Resource() {
        std::cout << "Release Resource: " << _id << std::endl;
    }

    size_t id() const { return _id; }

private:
    size_t _id;
};

static void reload_wrapper(Reloadable<Resource> *reloader) {
    while (reload_switch) {
        // reload per 10ms
        std::this_thread::sleep_for(Milliseconds(10));
        int ret = reloader->reload(nullptr);
        std::cout << "Reload Resource ret: " << ret << std::endl;
    }
}
 
int main(int argc, char** argv) {
    g_resource.init(nullptr);
    std::thread reload_thread(std::bind(&reload_wrapper, &g_resource));
 
    for (size_t i = 0; i < 10000; ++i) {
        ResourceHandle<Resource> handle = g_resource.get_resource();
        Resource* ptr1 = handle.get();
        assert(ptr1 != nullptr);
        std::cout << "Fetch resource: " << handle->id() << std::endl;
        std::this_thread::sleep_for(Milliseconds(13));
        Resource* ptr2 = handle.get();
        assert(ptr1 == ptr2);
    }

    reload_switch = false;
    reload_thread.join();
    return 0;
}
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
