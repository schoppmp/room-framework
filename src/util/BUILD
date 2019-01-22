package(default_visibility = ["//src:__subpackages__"])

cc_library(
    name = "util",
    hdrs = [
        "blocking_queue.hpp",
        "combine_pair.hpp",
        "get_ceil.hpp",
        "randomize_matrix.hpp",
        "serialize_le.hpp",
        "reservoir_sampling.hpp",
    ],
    deps = [
        ":time",
    ],
)

cc_library(
    name = "time",
    srcs = [
        "time.c",
    ],
    hdrs = [
        "time.h",
    ],
)
