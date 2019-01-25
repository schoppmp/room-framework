load("@io_bazel_rules_docker//container:bundle.bzl", "container_bundle")

container_bundle(
    name = "all_images",
    images = {
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/knn": "//sparse_linear_algebra/experiments/benchmark:knn_image",
    },
)
