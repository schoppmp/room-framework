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
        "@abseil//absl/strings",
        "@boost//:exception",
        "@boost//:range",
        "@boost//:serialization",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel",
        "@mpc_utils//mpc_utils:comm_channel_oblivc_adapter",
        "@mpc_utils//third_party/eigen",
        "@mpc_utils//third_party/ntl",
        "@oblivc//:runtime",
    ],
)
