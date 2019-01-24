package(default_visibility = ["//visibility:public"])

cc_library(
    name = "dense",
    hdrs = ["dense.hpp"],
    deps = [
        "//sparse_linear_algebra:pir_protocol",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:eigen",
    ],
)

alias(
    name = "sparse",
    actual = "//sparse_linear_algebra/matrix_multiplication/sparse:sparse",
)