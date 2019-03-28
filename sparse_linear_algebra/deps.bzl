load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)
load("@mpc_utils//mpc_utils:deps.bzl", "mpc_utils_deps")
load(
    "@io_bazel_rules_docker//cc:image.bzl",
    _cc_image_repos = "repositories",
)

all_content = """
filegroup(
  name = "all",
  srcs = glob(["**"]),
  visibility = ["//visibility:public"],
)
"""

# Sanitize a dependency so that it works correctly from code that includes
# this workspace as a git submodule.
def clean_dep(dep):
    return str(Label(dep))

def sparse_linear_algebra_deps():
    mpc_utils_deps()
    _cc_image_repos()

    if "fastpoly" not in native.existing_rules():
        http_archive(
            name = "fastpoly",
            url = "https://github.com/schoppmp/FastPolynomial/archive/19f5c2ac8a6a70942e81715276ce1b04f028e9dd.zip",
            sha256 = "8c376e1de025e1f54bda498e6e7f05e9eff827915a0144cedcf19248d2177291",
            strip_prefix = "FastPolynomial-19f5c2ac8a6a70942e81715276ce1b04f028e9dd",
            build_file = clean_dep("//third_party:fastpoly.BUILD"),
        )

    if "ack" not in native.existing_rules():
        http_archive(
            name = "ack",
            url = "https://bitbucket.org/jackdoerner/absentminded-crypto-kit/get/5230cef0887e.zip",
            sha256 = "133bfb1d5fdc2def0c32047e9f9288f71354ec56e003199b8bc516fb7a927fca",
            strip_prefix = "jackdoerner-absentminded-crypto-kit-5230cef0887e/src",
            build_file = clean_dep("//third_party:ack.BUILD"),
        )

    if "iterator_type_erasure" not in native.existing_rules():
        http_archive(
            name = "iterator_type_erasure",
            url = "http://thbecker.net/free_software_utilities/type_erasure_for_cpp_iterators/IteratorTypeErasure.zip",
            sha256 = "bd3891ea3b81f9e6ef238bf72ec1b82b018ecff08e9d5e5919839f14a52c4f23",
            strip_prefix = "IteratorTypeErasure",
            build_file = clean_dep("//third_party:iterator_type_erasure.BUILD"),
        )

    if "com_github_relic_toolkit_relic" not in native.existing_rules():
        http_archive(
            name = "com_github_relic_toolkit_relic",
            url = "https://github.com/relic-toolkit/relic/archive/b984e901ba78c83ea4093ea96addd13628c8c2d0.zip",
            sha256 = "34d66ea3e08e7b9496452c32941b7bc0e4c620d11f3f373d07e9ba1a2606f6ad",
            strip_prefix = "relic-b984e901ba78c83ea4093ea96addd13628c8c2d0",
            build_file_content = all_content,
        )

    if "boringssl" not in native.existing_rules():
        http_archive(
            name = "boringssl",
            sha256 = "1188e29000013ed6517168600fc35a010d58c5d321846d6a6dfee74e4c788b45",
            strip_prefix = "boringssl-7f634429a04abc48e2eb041c81c5235816c96514",
            urls = [
                "https://mirror.bazel.build/github.com/google/boringssl/archive/7f634429a04abc48e2eb041c81c5235816c96514.tar.gz",
                "https://github.com/google/boringssl/archive/7f634429a04abc48e2eb041c81c5235816c96514.tar.gz",
            ],
        )

    if "com_github_emp_toolkit_emp_tool" not in native.existing_rules():
        http_archive(
            name = "com_github_emp_toolkit_emp_tool",
            url = "https://github.com/emp-toolkit/emp-tool/archive/52495abff24d2510c203188c4953576590740dec.zip",
            sha256 = "198a2f67dc4de2157c22b87bf0f5df571cf1004bbf72aafeaeab5f11c263a7d5",
            strip_prefix = "emp-tool-52495abff24d2510c203188c4953576590740dec",
            build_file_content = all_content,
        )

    if "com_github_emp_toolkit_emp_ot" not in native.existing_rules():
        http_archive(
            name = "com_github_emp_toolkit_emp_ot",
            url = "https://github.com/adriagascon/emp-ot/archive/592c385e94f44fc942382b7c3d4b9f91ed84f33f.zip",
            sha256 = "bbe48c5414cdcc8dc23bce7dcca3c3c0c8a0a8c557110bea0615537af2cfc5cb",
            strip_prefix = "emp-ot-592c385e94f44fc942382b7c3d4b9f91ed84f33f",
            build_file = clean_dep("//third_party/emp:emp_ot.BUILD"),
        )