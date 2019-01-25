cc_binary(
    name = "pir",
    srcs = ["pir.cpp"],
    deps = [
        "//sparse_linear_algebra/oblivious_map:pir_protocol_basic",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_fss",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_poly",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_scs",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ],
)

cc_binary(
    name = "matrix_multiplication",
    srcs = ["matrix_multiplication.cpp"],
    deps = [
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/matrix_multiplication:sparse",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_basic",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_fss",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_poly",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_scs",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ],
)

cc_binary(
    name = "zero_sharing",
    srcs = ["zero_sharing.cpp"],
    deps = [
        "//sparse_linear_algebra/util",
        "//sparse_linear_algebra/zero_sharing",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ],
)
