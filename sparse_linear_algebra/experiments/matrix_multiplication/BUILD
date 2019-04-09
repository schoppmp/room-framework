package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

cc_binary(
    name = "matrix_multiplication",
    srcs = [
        "matrix_multiplication.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/matrix_multiplication:sparse",
        "//sparse_linear_algebra/matrix_multiplication/offline:fake_triple_provider",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/oblivious_map:basic_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:poly_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:sorting_oblivious_map",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:benchmarker",
        "@mpc_utils//mpc_utils:comm_channel",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_image(
    name = "main_image",
    binary = ":matrix_multiplication",
)

container_image(
    name = "image",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/matrix_multiplication/matrix_multiplication.ini",
    },
)
