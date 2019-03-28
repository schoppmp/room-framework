package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

cc_binary(
    name = "ot_triple_provider",
    srcs = [
        "ot_triple_provider.cpp",
    ],
    copts = [
        "-maes",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/matrix_multiplication/offline:ot_triple_provider",
        "@mpc_utils//mpc_utils:benchmarker",
        "@mpc_utils//mpc_utils:mpc_config",
    ],
)

cc_image(
    name = "ot_triple_provider_main_image",
    binary = ":ot_triple_provider",
)

container_image(
    name = "ot_triple_provider_image",
    base = ":ot_triple_provider_main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/matrix_multiplication/offline/ot_triple_provider.ini",
    },
)
