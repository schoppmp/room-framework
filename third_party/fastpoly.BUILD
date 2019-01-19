licenses(["notice"])  # MIT

cc_library(
    name = "fastpoly",
    srcs = [
        "iterative.cpp",
        "recursive.cpp",
        "utils.cpp",
        # Copy headers here so fastpoly itself builds without include_prefix.
        "iterative.h",
        "recursive.h",
        "utils.h",
    ],
    include_prefix = "fastpoly",
    hdrs = [
        "iterative.h",
        "recursive.h",
        "utils.h",
    ],
    visibility = ["//visibility:public"],
)
