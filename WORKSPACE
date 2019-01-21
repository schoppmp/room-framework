load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

# Use fork of rules_boost until https://github.com/nelhage/rules_boost/pull/103 is merged.
http_archive(
    name = "com_github_nelhage_rules_boost",
    url = "https://github.com/schoppmp/rules_boost/archive/86db51736cd2eaa709e5a1eba30d4e8c01789035.zip",
    sha256 = "49fe59038c1bdc528764a25ae7882c778eeabc2cba54e807666bdbb01cc99f93",
    strip_prefix = "rules_boost-86db51736cd2eaa709e5a1eba30d4e8c01789035",
)


load("//sparse-linear-algebra:preload.bzl", "sparse_linear_algebra_deps_preload")

sparse_linear_algebra_deps_preload()

load("//sparse-linear-algebra:deps.bzl", "sparse_linear_algebra_deps")

sparse_linear_algebra_deps()
