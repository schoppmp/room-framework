package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")

#load("@io_bazel_rules_docker//docker:docker.bzl", "docker_push")

cc_binary(
    name = "knn_binary",
    srcs = [
        "knn.cpp",
    ],
    data = [
        "knn.ini",
    ],
    deps = [
        "//sparse_linear_algebra/applications/knn:knn_protocol",
        "//sparse_linear_algebra/util",
    ],
)

cc_image(
    name = "knn_image",
    base = "@cc_image_base//image",
    binary = ":knn_binary",
)

## Pushing currently does not work when Python 3 is default:
## https://github.com/google/containerregistry/issues/42
#docker_push(
#    name = "knn_push_image",
#    image = ":knn_image",
#    registry = "gitlab.informatik.hu-berlin.de:4567",
#    repository = "ti/research/sparse-linear-algebra/room_old/knn",
#    tag = "{DOCKER_TAG}",
#)

cc_binary(
    name = "matrix_multiplication",
    srcs = [
        "matrix_multiplication.cpp",
    ],
    deps = [
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

cc_binary(
    name = "sgd",
    srcs = [
        "sgd.cpp",
    ],
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
