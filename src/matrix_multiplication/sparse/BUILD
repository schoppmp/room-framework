package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    hdrs = ["sparse_common.hpp"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "cols-dense",
    hdrs = ["cols-dense.hpp"],
    deps = [
        ":common",
        "//src:pir_protocol",
        "//src/matrix_multiplication:dense",
        "//src/util:time",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "cols-rows",
    hdrs = ["cols-rows.hpp"],
    deps = [
        ":common",
        "//src:pir_protocol",
        "//src/matrix_multiplication:dense",
        "//src/util:time",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)

cc_library(
    name = "rows-dense",
    hdrs = ["rows-dense.hpp"],
    deps = [
        ":common",
        "//src:pir_protocol",
        "//src/matrix_multiplication:dense",
        "//src/util:time",
        "//src/zero_sharing",
        "@boost//:range",
        "@mpc_utils//third_party/eigen",
    ],
)
