load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

licenses(["notice"])  # BSD

oblivc_library(
    name = "ack_native",
    srcs = glob([
        "src/**/*.c",
    ]),
    hdrs = glob([
        "src/**/*.h",
    ]),
)

oblivc_library(
    name = "ack",
    srcs = glob([
        "src/**/*.oc",
    ]),
    hdrs = glob([
        "src/**/*.oh",
    ]),
    deps = [
        ":ack_native",
    ],
)
