load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

oblivc_library(
    name = "knn",
    srcs = [
        "top_k.oc",
    ],
    hdrs = [
        "top_k.h",
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
        "//sparse_linear_algebra/matrix_multiplication/offline:fake_triple_provider",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/oblivious_map:basic_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:poly_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:sorting_oblivious_map",
        "//sparse_linear_algebra/util",
        "@com_google_absl//absl/strings",
        "@mpc_utils//mpc_utils:benchmarker",
        "@mpc_utils//mpc_utils:comm_channel_oblivc_adapter",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",
    ],
)
