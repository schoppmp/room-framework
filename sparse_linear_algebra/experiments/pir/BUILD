package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

cc_binary(
    name = "pir",
    srcs = [
        "pir.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/oblivious_map:basic_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:poly_oblivious_map",
        "//sparse_linear_algebra/oblivious_map:sorting_oblivious_map",
        "//sparse_linear_algebra/util",
        "@boost//:serialization",
        "@fastpoly",
        "@mpc_utils//mpc_utils:comm_channel",
        "@mpc_utils//mpc_utils:mpc_config",
        "@mpc_utils//mpc_utils/boost_serialization:ntl",
        "@mpc_utils//third_party/ntl",
    ],
)

cc_image(
    name = "main_image",
    binary = ":pir",
)

container_image(
    name = "image",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/pir/pir.ini",
    },
)
