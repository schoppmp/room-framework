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
            sha256 = "0ca82feb4acab59f42ebbf8544c959ef8e9a2a45550c07030c2f0d900c85e185",
            url = "https://github.com/schoppmp/rules_oblivc/archive/08a7ff3b836f14ac45f98eb6abf3004df8b1b59e.zip",
            strip_prefix = "rules_oblivc-08a7ff3b836f14ac45f98eb6abf3004df8b1b59e",
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/4a6fd7fd6228107cbfeba359c18367a16e5efbf8.zip",
            strip_prefix = "rules_foreign_cc-4a6fd7fd6228107cbfeba359c18367a16e5efbf8",
            sha256 = "f230217cd8f21632779581a5bd97e1bdd87cbe659e28e3d2cff8656127a52316",
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
            url = "https://github.com/schoppmp/mpc-utils/archive/c11d575da747a010e7fc1d6f4957b02061fcf96e.zip",
            sha256 = "78871f116d9680f83ac15b887e092cd09c82456ceac5cee2a92ccc7296f9f9e1",
            strip_prefix = "mpc-utils-c11d575da747a010e7fc1d6f4957b02061fcf96e",
        )

    if "io_bazel_rules_docker" not in native.existing_rules():
        http_archive(
            name = "io_bazel_rules_docker",
            sha256 = "ed884a780b82b0586b9c5454a2163b4133b2c50788f9334de4b6008780e26032",
            strip_prefix = "rules_docker-170335d284991ecc9fa5a6682c46bd32f167daa9",
            url = "https://github.com/bazelbuild/rules_docker/archive/170335d284991ecc9fa5a6682c46bd32f167daa9.zip",
        )
