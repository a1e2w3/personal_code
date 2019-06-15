cc_library(
    name = "wrpc",
    srcs = glob(["common/*.cpp", "common/*.cpp", "message/*.cpp", "network/*.cpp", "strategy/*.cpp", "utils/*.cpp"]),
    hdrs = glob(["common/*.h", "interface/*.h", "message/*.h", "network/*.h", "strategy/*.h", "utils/*.h"]),
    deps = [
        "@comlog//:main",
        "//thread_pool:thread_pool"
    ],
    copts = ["-g -O2 -std=c++11 -Iwrpc -Ithread_pool -Iexternal/comlog/comlog-master"],
    visibility = ["//visibility:public"],
    alwayslink = 1
)

cc_binary(
    name = "http_example",
    srcs = ["test/http_example.cpp", "test/mock_server.cpp", "test/mock_server.h"],
    deps = [
        ":wrpc",
        "@comlog//:main",
    ],
    defines = [],
    copts = ["-g -O2 -std=c++11 -Iwrpc -Ithread_pool -Iexternal/comlog"],
    linkopts = ["-lpthread"]
)

cc_binary(
    name = "example",
    srcs = ["test/example.cpp", "test/mock_server.cpp", "test/mock_server.h"],
    deps = [
        ":wrpc",
        "@comlog//:main",
    ],
    defines = [],
    copts = ["-g -O2 -std=c++11 -Iwrpc -Ithread_pool -Iexternal/comlog"],
    linkopts = ["-lpthread"]
)
