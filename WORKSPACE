load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)
load("//sparse_linear_algebra:preload.bzl", "sparse_linear_algebra_deps_preload")

sparse_linear_algebra_deps_preload()

load("//sparse_linear_algebra:deps.bzl", "sparse_linear_algebra_deps")

sparse_linear_algebra_deps()
