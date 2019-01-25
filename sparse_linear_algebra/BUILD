load("@io_bazel_rules_docker//container:bundle.bzl", "container_bundle")

container_bundle(
    name = "all_images",
    images = {
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/knn": "//sparse_linear_algebra/experiments/knn:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/pir": "//sparse_linear_algebra/experiments/pir:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/sgd": "//sparse_linear_algebra/experiments/sgd:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/matrix_multiplication": "//sparse_linear_algebra/experiments/matrix_multiplication:image",
    },
)
