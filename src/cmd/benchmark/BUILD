cc_binary(
    name = "knn",
    srcs = [
        "knn.cpp",
    ],
    deps = [
        # TODO: Merge with the following.
        "//src/util:knn",
    ],
)

cc_binary(
    name = "matrix_multiplication",
    srcs = [
        "matrix_multiplication.cpp",
    ],
    deps = [
        "//src/matrix_multiplication:dense",
        "//src/matrix_multiplication:sparse",
        "//src:pir_protocol",
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_binary(
    name = "pir",
    srcs = [
        "pir.cpp",
    ],
    deps = [
        "//src:pir_protocol",
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
        "@mpc_utils//third_party/ntl",
        "@fastpoly",
        "@boost//:serialization",
    ]
)

cc_binary(
    name = "sgd",
    srcs = [
        "sgd.cpp",
    ],
    deps = [
        "//src/applications:sigmoid",
        "//src/matrix_multiplication:dense",
        "//src/matrix_multiplication:sparse",
        "//src:pir_protocol",
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",

    ]

)