package(default_visibility = ["//visibility:public"])

load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

cc_library(
    name = "pir_protocol",
    hdrs = [
        "pir_protocol.hpp",
        "pir_protocol.tpp",
    ],
    deps = [
        "@boost//:iterator",
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
    deps = [
        ":pir_protocol",
        ":pir_protocol_basic_oblivc",
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
    deps = [
        ":pir_protocol",
        ":pir_protocol_fss_oblivc",
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
    deps = [
        ":pir_protocol",
        ":pir_protocol_poly_oblivc",
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
    deps = [
        ":pir_protocol",
        ":pir_protocol_scs_oblivc",
    ],
)
