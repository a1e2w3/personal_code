cc_library(
    name = "thread_pool",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    link_all_symbols=True,
)

cc_binary(
    name = "instance_pool_test",
    srcs = ["test/instance_pool_test.cpp"],
    defs = [],
    deps = [
        ":thread_pool",
    ],
)

cc_binary(
    name = "thread_pool_test",
    srcs = glob(["*.cpp", "test/main.cpp"]),
    deps = [
        "#pthread",
    ],
    defs = ["PERF_TEST"],
    extra_linkflags = []
)
