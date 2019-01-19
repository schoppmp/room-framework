cc_binary(
    name = "pir",
    srcs = ["pir.cpp"],
    copts = [
        "-DMPC_UTILS_USE_OBLIVC",
    ],
    deps = [
        "//src:pir_protocol_basic",
        "//src:pir_protocol_fss",
        "//src:pir_protocol_poly",
        "//src:pir_protocol_scs",
        "//src/util",
        "@ack//:oram_fssl",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
    ],
)
