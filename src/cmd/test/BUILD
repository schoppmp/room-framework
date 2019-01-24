cc_binary(
    name = "pir",
    srcs = ["pir.cpp"],
    deps = [
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ],
)

cc_binary(
    name = "matrix_multiplication",
    srcs = ["matrix_multiplication.cpp"],
    deps = [
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "//src/matrix_multiplication:dense",
        "//src/matrix_multiplication:sparse",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ]
)

cc_binary(
    name = "zero_sharing",
    srcs = ["zero_sharing.cpp"],
    deps = [
        "//src/zero_sharing",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
    ]
)