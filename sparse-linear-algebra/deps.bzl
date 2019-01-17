load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)
load("@com_github_schoppmp_mpc_utils//mpc-utils:deps.bzl", "mpc_utils_deps")

all_content = """
filegroup(
  name = "all",
  srcs = glob(["**"]),
  visibility = ["//visibility:public"]
)
"""

def sparse_linear_algebra_deps():
    mpc_utils_deps()

    if "com_github_schoppmp_fastpolynomial" not in native.existing_rules():
        http_archive(
            name = "com_github_schoppmp_fastpolynomial",
            url = "https://github.com/schoppmp/FastPolynomial/archive/19f5c2ac8a6a70942e81715276ce1b04f028e9dd.zip",
            sha256 = "8c376e1de025e1f54bda498e6e7f05e9eff827915a0144cedcf19248d2177291",
            strip_prefix = "FastPolynomial-19f5c2ac8a6a70942e81715276ce1b04f028e9dd",
            build_file = "//third_party:fastpoly.BUILD",
        )

    if "org_bitbucket_jackdoerner_ack" not in native.existing_rules():
        http_archive(
            name = "org_bitbucket_jackdoerner_ack",
            url = "https://bitbucket.org/jackdoerner/absentminded-crypto-kit/get/5230cef0887e.zip",
            sha256 = "133bfb1d5fdc2def0c32047e9f9288f71354ec56e003199b8bc516fb7a927fca",
            strip_prefix = "jackdoerner-absentminded-crypto-kit-5230cef0887e/src",
            build_file = "//third_party:ack.BUILD",
        )
