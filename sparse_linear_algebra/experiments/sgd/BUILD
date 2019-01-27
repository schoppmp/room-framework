package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

cc_binary(
    name = "sgd",
    srcs = [
        "sgd.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/applications/sgd:sigmoid",
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

cc_image(
    name = "main_image",
    base = "@cc_image_base//image",
    binary = ":sgd",
)

container_image(
    name = "image",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/sgd/sgd.ini",
    },
)
