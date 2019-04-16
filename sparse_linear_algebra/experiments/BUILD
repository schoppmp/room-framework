load("@io_bazel_rules_docker//container:bundle.bzl", "container_bundle")

container_bundle(
    name = "all_images",
    images = {
        # Generic examples
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/knn": "//sparse_linear_algebra/experiments/knn:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/knn_non_uniform_sparsity": "//sparse_linear_algebra/experiments/knn:image_non_uniform_sparsity",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/pir": "//sparse_linear_algebra/experiments/pir:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/sgd": "//sparse_linear_algebra/experiments/sgd:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/matrix_multiplication": "//sparse_linear_algebra/experiments/matrix_multiplication:image",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/matrix_multiplication/ot_triple_provider": "//sparse_linear_algebra/experiments/matrix_multiplication/offline:ot_triple_provider_image",

        # ROOM paper
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/knn/room_time": "//sparse_linear_algebra/experiments/knn:room_time",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/sgd/room_communication": "//sparse_linear_algebra/experiments/sgd:room_communication",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/sgd/room_time": "//sparse_linear_algebra/experiments/sgd:room_time",
        "gitlab.informatik.hu-berlin.de:4567/ti/research/sparse-linear-algebra/sgd/room_sparsity": "//sparse_linear_algebra/experiments/sgd:room_sparsity",
    },
)
