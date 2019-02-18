package(default_visibility = ["//visibility:public"])

cc_library(
    name = "dense",
    hdrs = ["dense.hpp"],
    deps = [
        "//sparse_linear_algebra/matrix_multiplication/offline:triple_provider",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/util",
        "@mpc_utils//mpc_utils:comm_channel_oblivc",
        "@mpc_utils//mpc_utils/boost_serialization:eigen",
    ],
)

cc_library(
    name = "sparse",
    deps = [
        ":cols-dense",
        ":cols-rows",
        ":rows-dense",
    ],
)

cc_library(
    name = "sparse_common",
    hdrs = ["sparse_common.hpp"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "cols-dense",
    hdrs = ["cols-dense.hpp"],
    deps = [
        ":sparse_common",
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/util:time",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "cols-rows",
    hdrs = ["cols-rows.hpp"],
    deps = [
        ":sparse_common",
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/util:time",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "rows-dense",
    hdrs = ["rows-dense.hpp"],
    deps = [
        ":sparse_common",
        "//sparse_linear_algebra/matrix_multiplication:dense",
        "//sparse_linear_algebra/oblivious_map",
        "//sparse_linear_algebra/util:time",
        "//sparse_linear_algebra/zero_sharing",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)
