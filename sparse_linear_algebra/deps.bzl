load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)
load("@mpc_utils//third_party:repo.bzl", "third_party_http_archive")
load("@mpc_utils//mpc_utils:deps.bzl", "mpc_utils_deps")
load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)
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
    # Load EMP-OT before mpc_utils_deps() because we need a patched version.
    # TODO(adria) merge this version upstream or put the patch in this repo.
    if "com_github_emp_toolkit_emp_ot" not in native.existing_rules():
        third_party_http_archive(
            name = "com_github_emp_toolkit_emp_ot",
            build_file = "@mpc_utils//third_party:emp_ot.BUILD",
            link_files = {
                "@mpc_utils//third_party/emp-tool:FindBoost.cmake": "cmake/FindBoost.cmake",
            },
            sha256 = "8cc43964b0d1880429acf6dd972f1e080923c71d25127e74607f628b7cdc6c30",
            strip_prefix = "emp-ot-90319da396e731d0b24e90e1ccaa3b76fa73f641",
            url = "https://github.com/adriagascon/emp-ot/archive/90319da396e731d0b24e90e1ccaa3b76fa73f641.zip",
        )

    mpc_utils_deps()
    container_repositories()
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
