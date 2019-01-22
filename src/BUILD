load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

cc_library(
    name = "pir_protocol",
    hdrs = [
        "pir_protocol.hpp",
        "pir_protocol.tpp",
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
        "pir_protocol_basic.oc",
    ],
    hdrs = [
        "pir_protocol_basic.h",
    ],
    deps = [
        "@ack//:oaes",
    ],
)

cc_library(
    name = "pir_protocol_basic",
    hdrs = [
        "pir_protocol_basic.hpp",
        "pir_protocol_basic.tpp",
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
        "pir_protocol_fss.oc",
    ],
    hdrs = [
        "pir_protocol_fss.h",
    ],
    deps = [
        "//src/util:time",
        "@ack//:oram_fssl",
    ],
)

cc_library(
    name = "pir_protocol_fss",
    hdrs = [
        "pir_protocol_fss.hpp",
        "pir_protocol_fss.tpp",
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
        "pir_protocol_poly.oc",
    ],
    hdrs = [
        "pir_protocol_poly.h",
    ],
    deps = [
        "@ack//:oaes",
    ],
)

cc_library(
    name = "pir_protocol_poly",
    hdrs = [
        "pir_protocol_poly.hpp",
        "pir_protocol_poly.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_poly_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
        "@fastpoly",
    ],
)

oblivc_library(
    name = "pir_protocol_scs_oblivc",
    srcs = [
        "pir_protocol_scs.oc",
    ],
    hdrs = [
        "pir_protocol_scs.h",
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
        "pir_protocol_scs.hpp",
        "pir_protocol_scs.tpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":pir_protocol",
        ":pir_protocol_scs_oblivc",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)
