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
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/matrix_multiplication:sparse",
        "//sparse_linear_algebra/oblivious_map:pir_protocol",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_basic",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_fss",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_poly",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_scs",
        "//sparse_linear_algebra/util",
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
