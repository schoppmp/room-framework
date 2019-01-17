load("@com_github_schoppmp_rules_oblivc//oblivc:oblivc.bzl", "oblivc_library")

licenses(["notice"])  # BSD

oblivc_library(
    name = "oram_ckt_block",
    srcs = [
        "oram_ckt/block.oc",
    ],
    hdrs = [
        "oram_ckt/block.oh",
    ],
    deps = [
        ":oram_ckt_utils",
    ],
)

oblivc_library(
    name = "oram_ckt_linear_scan_oram",
    srcs = [
        "oram_ckt/linear_scan_oram.oc",
    ],
    hdrs = [
        "oram_ckt/linear_scan_oram.oh",
    ],
    deps = [
        ":oram_ckt_utils",
    ],
)

oblivc_library(
    name = "oram_ckt_nonrecursive_oram",
    srcs = [
        "oram_ckt/nonrecursive_oram.oc",
    ],
    hdrs = [
        "oram_ckt/nonrecursive_oram.oh",
    ],
    deps = [
        ":oram_ckt_block",
    ],
)

oblivc_library(
    name = "oram_ckt_utils",
    srcs = [
        "oram_ckt/utils.oc",
    ],
    hdrs = [
        "oram_ckt/utils.oh",
    ],
)

oblivc_library(
    name = "oram_ckt",
    srcs = [
        "oram_ckt/circuit_oram.oc",
    ],
    hdrs = [
        "oram_ckt/circuit_oram.oh",
    ],
    deps = [
        ":oram_ckt_block",
        ":oram_ckt_linear_scan_oram",
        ":oram_ckt_nonrecursive_oram",
        ":oram_ckt_utils",
    ],
)

cc_library(
    name = "aes_gladman",
    srcs = [
        "oram_fssl/aes_gladman/aescrypt.c",
        "oram_fssl/aes_gladman/aeskey.c",
        "oram_fssl/aes_gladman/aestab.c",
    ],
    hdrs = [
        "oram_fssl/aes_gladman/aes.h",
        "oram_fssl/aes_gladman/aesopt.h",
        "oram_fssl/aes_gladman/aestab.h",
        "oram_fssl/aes_gladman/brg_endian.h",
        "oram_fssl/aes_gladman/brg_types.h",
    ],
)

# Lots of circular includes in this one, which is why we have to compile
# everything at once.
cc_library(
    name = "oram_fssl_native",
    srcs = [
        "oram_fssl/floram_util.c",
        "oram_fssl/fss.c",
        "oram_fssl/fss_cprg.c",
        "oram_fssl/scanrom.c",
        # Internal headers.
        "oram_fssl/floram_util.h",
        "oram_fssl/scanrom.h",
    ],
    hdrs = [
        "oram_fssl/floram.h",
        "oram_fssl/fss.h",
        "oram_fssl/fss_cprg.h",
    ],
    copts = [
        "-fopenmp",
    ],
    deps = [
        ":ackutil",
        ":aes_gladman",
    ],
)

oblivc_library(
    name = "oram_fssl",
    srcs = [
        "oram_fssl/floram.oc",
        "oram_fssl/floram_util.oc",
        "oram_fssl/fss.oc",
        "oram_fssl/fss_cprg.oc",
        "oram_fssl/scanrom.oc",
        # Internal headers.
        "oram_fssl/floram_util.oh",
        "oram_fssl/scanrom.oh",
    ],
    hdrs = [
        "oram_fssl/floram.oh",
        "oram_fssl/fss.oh",
        "oram_fssl/fss_cprg.oh",
    ],
    deps = [
        ":ackutil",
        ":endian",
        ":oaes",
        ":oram_fssl_native",
    ],
)

oblivc_library(
    name = "oram_sqrt",
    srcs = [
        "oram_sqrt/decoder.oc",
        "oram_sqrt/shuffle.oc",
        "oram_sqrt/sqrtoram.oc",
        # Internal headers.
        "oram_sqrt/decoder.oh",
        "oram_sqrt/shuffle.oh",
    ],
    hdrs = [
        "oram_sqrt/sqrtoram.oh",
    ],
    deps = [
        ":shuffle",
        ":waksman",
    ],
)

cc_library(
    name = "ackutil",
    srcs = [
        "ackutil.c",
    ],
    hdrs = [
        "ackutil.h",
    ],
    deps = [
        "@oblivc//:runtime",
    ],
)

oblivc_library(
    name = "endian",
    srcs = [
        "endian.oc",
    ],
    hdrs = [
        "endian.oh",
    ],
)

oblivc_library(
    name = "oaes",
    srcs = [
        "oaes.oc",
    ],
    hdrs = [
        "oaes.oh",
    ],
)

oblivc_library(
    name = "obig",
    srcs = [
        "obig.oc",
    ],
    hdrs = [
        "obig.oh",
    ],
    deps = [
        ":ackutil",
    ],
)

oblivc_library(
    name = "ochacha",
    srcs = [
        "ochacha.oc",
    ],
    hdrs = [
        "ochacha.oh",
    ],
)

oblivc_library(
    name = "ograph",
    srcs = [
        "ograph.oc",
    ],
    hdrs = [
        "ograph.oh",
    ],
    deps = [
        ":oqueue",
        ":oram",
        ":osort",
    ],
)

oblivc_library(
    name = "oqueue",
    srcs = [
        "oqueue.oc",
    ],
    hdrs = [
        "oqueue.oh",
    ],
)

oblivc_library(
    name = "oram",
    srcs = [
        "oram.oc",
    ],
    hdrs = [
        "oram.oh",
    ],
    deps = [
        ":oram_ckt",
        ":oram_fssl",
        ":oram_sqrt",
    ],
)

oblivc_library(
    name = "osalsa",
    srcs = [
        "osalsa.oc",
    ],
    hdrs = [
        "osalsa.oh",
    ],
)

oblivc_library(
    name = "oscrypt",
    srcs = [
        "oscrypt.oc",
    ],
    hdrs = [
        "oscrypt.oh",
    ],
    deps = [
        ":endian",
        ":oram",
        ":osalsa",
        ":osha256",
    ],
)

oblivc_library(
    name = "osearch",
    srcs = [
        "osearch.oc",
    ],
    hdrs = [
        "osearch.oh",
    ],
    deps = [
        ":oram",
    ],
)

oblivc_library(
    name = "osha256",
    srcs = [
        "osha256.oc",
    ],
    hdrs = [
        "osha256.oh",
    ],
    deps = [
        ":endian",
    ],
)

oblivc_library(
    name = "osha512",
    srcs = [
        "osha512.oc",
    ],
    hdrs = [
        "osha512.oh",
    ],
    deps = [
        ":endian",
    ],
)

oblivc_library(
    name = "osort",
    srcs = [
        "osort.oc",
    ],
    hdrs = [
        "osort.oh",
    ],
)

oblivc_library(
    name = "shuffle",
    srcs = [
        "shuffle.oc",
    ],
    hdrs = [
        "shuffle.oh",
    ],
    deps = [
        ":waksman",
    ],
)

cc_library(
    name = "uint128",
    srcs = [
        "uint128.c",
    ],
    hdrs = [
        "uint128.h",
    ],
)

cc_library(
    name = "waksman",
    srcs = [
        "waksman.c",
    ],
    hdrs = [
        "waksman.h",
    ],
)
