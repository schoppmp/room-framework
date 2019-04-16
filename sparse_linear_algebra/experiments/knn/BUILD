package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

#load("@io_bazel_rules_docker//docker:docker.bzl", "docker_push")

cc_library(
    name = "knn_config",
    srcs = ["knn_config.cpp"],
    hdrs = ["knn_config.hpp"],
    deps = [
        "//sparse_linear_algebra/applications/knn:knn_protocol",
        "//sparse_linear_algebra/util",
    ]
)

cc_binary(
    name = "knn",
    srcs = [
        "knn.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/applications/knn:knn_protocol",
        "//sparse_linear_algebra/util",
        ":knn_config",
    ],
)

cc_binary(
    name = "knn_non_uniform_sparsity",
    srcs = [
        "knn_non_uniform_sparsity.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/applications/knn:knn_protocol",
        "//sparse_linear_algebra/util",
        ":knn_config",
    ],
)

cc_image(
    name = "main_image",
    binary = ":knn",
)

cc_image(
    name = "main_image_non_uniform_sparsity",
    binary = ":knn_non_uniform_sparsity",
)

container_image(
    name = "image",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/knn/knn.ini",
    },
)

container_image(
    name = "image_non_uniform_sparsity",
    base = ":main_image_non_uniform_sparsity",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/knn/knn_non_uniform_sparsity.ini",
    },
)

container_image(
    name = "room_time",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/knn/room_time.ini",
    },
)

## Pushing currently does not work when Python 3 is default:
## https://github.com/google/containerregistry/issues/42
#docker_push(
#    name = "knn_push_image",
#    image = ":image",
#    registry = "gitlab.informatik.hu-berlin.de:4567",
#    repository = "ti/research/sparse-linear-algebra/room_old/knn",
#    tag = "{DOCKER_TAG}",
#)
