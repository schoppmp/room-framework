package(default_visibility = ["//visibility:public"])

cc_library(
    name = "triple_provider",
    hdrs = [
        "triple_provider.hpp",
    ],
    deps = [
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "fake_triple_provider",
    hdrs = [
        "fake_triple_provider.hpp",
    ],
    textual_hdrs = [
        "fake_triple_provider.tpp",
    ],
    deps = [
        ":triple_provider",
        "//sparse_linear_algebra/util",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "ot_triple_provider",
    hdrs = [
        "ot_triple_provider.hpp",
    ],
    textual_hdrs = [
        "ot_triple_provider.tpp",
    ],
    deps = [
        ":triple_provider",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
    ],
)

cc_test(
    name = "fake_triple_provider_test",
    size = "small",
    srcs = [
        "fake_triple_provider_test.cpp",
    ],
    deps = [
        ":fake_triple_provider",
        "@mpc_utils//mpc_utils/testing:test_deps",
    ],
)

cc_test(
    name = "ot_triple_provider_test",
    srcs = [
        "ot_triple_provider_test.cpp",
    ],
    deps = [
        ":ot_triple_provider",
        "@mpc_utils//mpc_utils/testing:comm_channel_test_helper_oblivc",
        "@mpc_utils//mpc_utils/testing:test_deps",
    ],
)
