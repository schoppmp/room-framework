load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

# Dependencies that need to be defined before :deps.bzl can be loaded.
# Copy those in a similar preload.bzl file in any workspace that depends on
# this one.
def sparse_linear_algebra_deps_preload():
    # Transitive dependencies from mpc-utils.
    if "com_github_nelhage_rules_boost" not in native.existing_rules():
        http_archive(
            name = "com_github_nelhage_rules_boost",
            url = "https://github.com/nelhage/rules_boost/archive/691a53dd7dc4fb8ab70f61acad9b750a1bf10dc6.zip",
            sha256 = "5837d6bcf96c60dc1407126e828287098f91a8c69d8c2ccf8ebb0282ed35b401",
            strip_prefix = "rules_boost-691a53dd7dc4fb8ab70f61acad9b750a1bf10dc6",
        )

    if "com_github_schoppmp_rules_oblivc" not in native.existing_rules():
        http_archive(
            name = "com_github_schoppmp_rules_oblivc",
            sha256 = "5a9fd34b0402851cabc6459cebaecfe00887030025c60f416032fbb4073be97f",
            url = "https://github.com/schoppmp/rules_oblivc/archive/bb5afcac02fbdb79c52b2e963ebff4843fdabf79.zip",
            strip_prefix = "rules_oblivc-bb5afcac02fbdb79c52b2e963ebff4843fdabf79",
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/216ded8acb95d81e312b228dce3c39872c7a7c34.zip",
            strip_prefix = "rules_foreign_cc-216ded8acb95d81e312b228dce3c39872c7a7c34",
            sha256 = "bb38d30c5d06cc1aedc9db7d2274d2323419a60200ac8e5fdbdc100e37740975",
        )

    # New dependencies.
    if "mpc_utils" not in native.existing_rules():
        http_archive(
            name = "mpc_utils",
            url = "https://github.com/schoppmp/mpc-utils/archive/ba1b34e630cd9a6b70a152234e8ece0e24cff291.zip",
            sha256 = "e63cba0c498322059c917ae02811fe820a7ced0ec648954afb40e1ee538d4e6b",
            strip_prefix = "mpc-utils-ba1b34e630cd9a6b70a152234e8ece0e24cff291",
        )
