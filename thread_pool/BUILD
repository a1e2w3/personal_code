cc_library(
    name = "thread_pool",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    copts = ["-g -O2 -std=c++11"],
    visibility = ["//visibility:public"],
    alwayslink = 1
)

cc_binary(
    name = "instance_pool_test",
    srcs = ["test/instance_pool_test.cpp"],
    deps = [],
    defines = ["__DEBUG_INSTANCE_POOL__"],
    copts = ["-g -O2 -std=c++11"],
)

cc_binary(
    name = "thread_pool_test",
    srcs = glob(["*.cpp", "*.h", "test/main.cpp"]),
    deps = [],
    defines = ["PERF_TEST"],
    copts = ["-g -O2 -std=c++11"],
    linkopts = []
)
