package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")

cc_binary(
    name = "pir",
    srcs = [
        "pir.cpp",
    ],
    deps = [
        "//sparse_linear_algebra/oblivious_map:pir_protocol",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_basic",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_fss",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_poly",
        "//sparse_linear_algebra/oblivious_map:pir_protocol_scs",
        "//sparse_linear_algebra/util",
        "@boost//:serialization",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
        "@mpc_utils//third_party/ntl",
    ],
)

cc_image(
    name = "image",
    base = "@cc_image_base//image",
    binary = ":pir",
)
