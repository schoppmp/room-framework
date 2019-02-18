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

cc_test(
    name = "fake_triple_provider_test",
    srcs = [
        "fake_triple_provider_test.cpp",
    ],
    deps = [
        ":fake_triple_provider",
        "@googletest//:gtest_main",
    ],
)
