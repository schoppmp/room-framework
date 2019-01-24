package(default_visibility = ["//visibility:public"])

cc_library(
    name = "dense",
    hdrs = ["dense.hpp"],
    deps = [
        "//src:pir_protocol",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:eigen",
    ],
)

alias(
    name = "sparse",
    actual = "//src/matrix_multiplication/sparse:sparse",
)