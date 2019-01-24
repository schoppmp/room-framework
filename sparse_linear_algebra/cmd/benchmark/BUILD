cc_binary(
    name = "knn",
    srcs = [
        "knn.cpp",
    ],
    deps = [
        # TODO: Merge with the following.
        "//sparse_linear_algebra/util:knn",
    ],
)

cc_binary(
    name = "matrix_multiplication",
    srcs = [
        "matrix_multiplication.cpp",
    ],
    deps = [
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/matrix_multiplication:sparse",
        "//sparse_linear_algebra:pir_protocol",
        "//sparse_linear_algebra:pir_protocol_basic",
        "//sparse_linear_algebra:pir_protocol_fss",
        "//sparse_linear_algebra:pir_protocol_poly",
        "//sparse_linear_algebra:pir_protocol_scs",
        "//sparse_linear_algebra/util",
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
        "//sparse_linear_algebra:pir_protocol",
        "//sparse_linear_algebra:pir_protocol_basic",
        "//sparse_linear_algebra:pir_protocol_fss",
        "//sparse_linear_algebra:pir_protocol_poly",
        "//sparse_linear_algebra:pir_protocol_scs",
        "//sparse_linear_algebra/util",
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
        "//sparse_linear_algebra/applications:sigmoid",
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/matrix_multiplication:sparse",
        "//sparse_linear_algebra:pir_protocol",
        "//sparse_linear_algebra:pir_protocol_basic",
        "//sparse_linear_algebra:pir_protocol_fss",
        "//sparse_linear_algebra:pir_protocol_poly",
        "//sparse_linear_algebra:pir_protocol_scs",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",

    ]

)