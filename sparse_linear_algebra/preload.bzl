load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

# Dependencies that need to be defined before :deps.bzl can be loaded.
# Copy those in a similar preload.bzl file in any workspace that depends on
# this one.
def sparse_linear_algebra_deps_preload():
    # Transitive dependencies of mpc_utils.
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
            sha256 = "ab5f324e741ea70c7f5ebcd5d84710c4fd80a952211b86e68c3e0338fd2b39e0",
            url = "https://github.com/schoppmp/rules_oblivc/archive/11ee07428193eadd0a807c725e379de2c2574d95.zip",
            strip_prefix = "rules_oblivc-11ee07428193eadd0a807c725e379de2c2574d95",
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/216ded8acb95d81e312b228dce3c39872c7a7c34.zip",
            strip_prefix = "rules_foreign_cc-216ded8acb95d81e312b228dce3c39872c7a7c34",
            sha256 = "bb38d30c5d06cc1aedc9db7d2274d2323419a60200ac8e5fdbdc100e37740975",
        )

    # Transitive dependency of io_bazel_rules_docker
    if "bazel_skylib" not in native.existing_rules():
        http_archive(
            name = "bazel_skylib",
            sha256 = "c33a54ef16961e48df7d306a67ccb92c0c4627d0549df519e07533a6f3d270aa",
            strip_prefix = "bazel-skylib-9b85311ab47548ec051171213a1b3b4b3b3c9dc8",
            url = "https://github.com/bazelbuild/bazel-skylib/archive/9b85311ab47548ec051171213a1b3b4b3b3c9dc8.zip",
        )

    # New dependencies.
    if "mpc_utils" not in native.existing_rules():
        http_archive(
            name = "mpc_utils",
            url = "https://github.com/schoppmp/mpc-utils/archive/0aa90ef1d01e7f4a46fae56ff8991a3edb1063bf.zip",
            sha256 = "f787b291a68e29dbe952f1ccafcb67c0991588b2a516bca39e891392d59800c5",
            strip_prefix = "mpc-utils-0aa90ef1d01e7f4a46fae56ff8991a3edb1063bf",
        )

    if "io_bazel_rules_docker" not in native.existing_rules():
        http_archive(
            name = "io_bazel_rules_docker",
            sha256 = "ed884a780b82b0586b9c5454a2163b4133b2c50788f9334de4b6008780e26032",
            strip_prefix = "rules_docker-170335d284991ecc9fa5a6682c46bd32f167daa9",
            url = "https://github.com/bazelbuild/rules_docker/archive/170335d284991ecc9fa5a6682c46bd32f167daa9.zip",
        )
