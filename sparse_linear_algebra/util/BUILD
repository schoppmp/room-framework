package(default_visibility = ["//sparse_linear_algebra:__subpackages__"])

cc_library(
    name = "util",
    deps = [
        ":blocking_queue",
        ":combine_pair",
        ":get_ceil",
        ":randomize_matrix",
        ":reservoir_sampling",
        ":serialize_le",
        ":time",
    ],
)

cc_library(
    name = "blocking_queue",
    hdrs = [
        "blocking_queue.hpp",
    ],
)

cc_library(
    name = "combine_pair",
    hdrs = [
        "combine_pair.hpp",
    ],
)

cc_library(
    name = "get_ceil",
    hdrs = [
        "get_ceil.hpp",
    ],
)

cc_library(
    name = "randomize_matrix",
    hdrs = [
        "randomize_matrix.hpp",
    ],
)

cc_library(
    name = "serialize_le",
    hdrs = [
        "serialize_le.hpp",
    ],
)

cc_library(
    name = "reservoir_sampling",
    hdrs = [
        "reservoir_sampling.hpp",
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

# TODO: This shouldn't be in util
cc_library(
    name = "knn",
    srcs = [
        "knn.cpp",
    ],
    hdrs = [
        "knn.hpp",
    ],
    deps = [
        ":util",
        "//sparse_linear_algebra/applications:knn_protocol",
    ],
)
