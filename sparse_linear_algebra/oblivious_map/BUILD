load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

cc_library(
    name = "pir_protocol",
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
    name = "pir_protocol_basic_oblivc",
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
    name = "pir_protocol_basic",
    hdrs = [
        "basic_oblivious_map.hpp",
        "basic_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_basic_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)

oblivc_library(
    name = "pir_protocol_fss_oblivc",
    srcs = [
        "fss_oblivious_map.oc",
    ],
    hdrs = [
        "fss_oblivious_map.h",
    ],
    deps = [
        "//sparse_linear_algebra/util:time",
        "@ack//:oram_fssl",
    ],
)

cc_library(
    name = "pir_protocol_fss",
    hdrs = [
        "fss_oblivious_map.hpp",
        "fss_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_fss_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)

oblivc_library(
    name = "pir_protocol_poly_oblivc",
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
    name = "pir_protocol_poly",
    hdrs = [
        "poly_oblivious_map.hpp",
        "poly_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_poly_oblivc",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
    ],
)

oblivc_library(
    name = "pir_protocol_scs_oblivc",
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
    name = "pir_protocol_scs",
    hdrs = [
        "sorting_oblivious_map.hpp",
        "sorting_oblivious_map.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_scs_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)
