load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

cc_library(
    name = "oblivious_map",
    hdrs = [
        "oblivious_map.hpp",
        "oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@boost//:iterator",
        "@boost//:range",
        "@iterator_type_erasure//:any_iterator",
    ],
)

oblivc_library(
    name = "basic_oblivious_map_oblivc",
    srcs = [
        "basic_oblivious_map.oc",
    ],
    hdrs = [
        "basic_oblivious_map.h",
    ],
    deps = [
        "@ack//:oaes",
    ],
)

cc_library(
    name = "basic_oblivious_map",
    hdrs = [
        "basic_oblivious_map.hpp",
        "basic_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":basic_oblivious_map_oblivc",
        ":oblivious_map",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)

oblivc_library(
    name = "poly_oblivious_map_oblivc",
    srcs = [
        "poly_oblivious_map.oc",
    ],
    hdrs = [
        "poly_oblivious_map.h",
    ],
    deps = [
        "@ack//:oaes",
    ],
)

cc_library(
    name = "poly_oblivious_map",
    hdrs = [
        "poly_oblivious_map.hpp",
        "poly_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":oblivious_map",
        ":poly_oblivious_map_oblivc",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
    ],
)

oblivc_library(
    name = "sorting_oblivious_map_oblivc",
    srcs = [
        "sorting_oblivious_map.oc",
    ],
    hdrs = [
        "sorting_oblivious_map.h",
    ],
    deps = [
        "@ack//:oaes",
        "@ack//:obig",
        "@ack//:osort",
        "@ack//:shuffle",
    ],
)

cc_library(
    name = "sorting_oblivious_map",
    hdrs = [
        "sorting_oblivious_map.hpp",
        "sorting_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":oblivious_map",
        ":sorting_oblivious_map_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)
