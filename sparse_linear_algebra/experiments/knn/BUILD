package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")
load("@io_bazel_rules_docker//container:image.bzl", "container_image")

#load("@io_bazel_rules_docker//docker:docker.bzl", "docker_push")

cc_binary(
    name = "knn",
    srcs = [
        "knn.cpp",
    ],
    data = glob(["*.ini"]),
    deps = [
        "//sparse_linear_algebra/applications/knn:knn_protocol",
        "//sparse_linear_algebra/util",
    ],
)

cc_image(
    name = "main_image",
    binary = ":knn",
)

container_image(
    name = "image",
    base = ":main_image",
    env = {
        "MPC_UTILS_CONFIG": "sparse_linear_algebra/experiments/knn/knn.ini",
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
