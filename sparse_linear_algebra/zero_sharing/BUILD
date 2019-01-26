load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

oblivc_library(
    name = "zero_sharing_oblivc",
    srcs = ["zero_sharing.oc"],
    hdrs = ["zero_sharing.h"],
    deps = [
        "@ack//:oaes",
    ],
)

cc_library(
    name = "zero_sharing",
    hdrs = ["zero_sharing.hpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":zero_sharing_oblivc",
        "@mpc_utils//third_party/ntl",
        "@mpc_utils//third_party/eigen",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@boost//:serialization",
        "@boost//:range",
        "@boost//:exception",
        "@fastpoly",
        "@oblivc//:runtime",
    ],
)
