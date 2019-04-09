package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # MIT

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
        "@abseil//absl/memory",
        "@boost//:throw_exception",
        "@mpc_utils//mpc_utils:comm_channel",
        "@mpc_utils//mpc_utils:comm_channel_emp_adapter",
        "@mpc_utils//mpc_utils:openssl_uniform_bit_generator",
        "@mpc_utils//mpc_utils:statusor",
        "@mpc_utils//third_party/emp:emp_ot",
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
    # These copts need to be passed to anything that depends on EMP.
    copts = [
        "-maes",
        "-msse4.2",
    ],
    deps = [
        ":ot_triple_provider",
        "@mpc_utils//mpc_utils:comm_channel",
        "@mpc_utils//mpc_utils:status_matchers",
        "@mpc_utils//mpc_utils:statusor",
        "@mpc_utils//mpc_utils/boost_serialization:eigen",
        "@mpc_utils//mpc_utils/testing:comm_channel_test_helper",
        "@mpc_utils//mpc_utils/testing:test_deps",
    ],
)
