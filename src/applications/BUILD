load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")


oblivc_library(
    name = "knn",
    srcs = [
        "knn.oc",
    ],
    hdrs = [
        "knn.h",
    ],
)

cc_library(
    name = "knn_protocol",
    srcs = [
        "knn_protocol.cpp",
    ],
    hdrs = [
        "knn_protocol.hpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":knn",
        "//src:pir_protocol",
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/matrix_multiplication:dense",
        "//src/matrix_multiplication:sparse",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",
    ],
)

oblivc_library(
    name = "sigmoid",
    srcs = [
        "sigmoid.oc",
    ],
    hdrs = [
        "sigmoid.h",
    ],
    visibility = ["//visibility:public"],
)
