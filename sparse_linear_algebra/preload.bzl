load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

# Dependencies that need to be defined before :deps.bzl can be loaded.
# Copy those in a similar preload.bzl file in any workspace that depends on
# this one.
def sparse_linear_algebra_deps_preload():
    # Transitive dependencies of mpc_utils.
    if "com_google_absl" not in native.existing_rules():
        http_archive(
            name = "com_google_absl",
            urls = ["https://github.com/abseil/abseil-cpp/archive/ccdd1d57b6386ebc26fb0c7d99b604672437c124.zip"],
            strip_prefix = "abseil-cpp-ccdd1d57b6386ebc26fb0c7d99b604672437c124",
            sha256 = "a463a791e1b5eaad461956495401357efb792fcdbf47b5737ec420bb54c804b6",
        )
    if "com_github_nelhage_rules_boost" not in native.existing_rules():
        http_archive(
            name = "com_github_nelhage_rules_boost",
            sha256 = "ea88159d5b91a852de0cd8ccdc78c9ca42c54538241aa7ff727de3544da7f051",
            strip_prefix = "rules_boost-fe9a0795e909f10f2bfb6bfa4a51e66641e36557",
            url = "https://github.com/nelhage/rules_boost/archive/fe9a0795e909f10f2bfb6bfa4a51e66641e36557.zip",
        )
    if "com_github_schoppmp_rules_oblivc" not in native.existing_rules():
        http_archive(
            name = "com_github_schoppmp_rules_oblivc",
            sha256 = "10f53f0fab3e374fdef83dc4279d902a357993a87c91d6de3c30e64db59f87ee",
	        url = "https://github.com/schoppmp/rules_oblivc/archive/d33ab3a707b9d6e9a79a683e660e572ab8e92f16.zip",
            strip_prefix = "rules_oblivc-d33ab3a707b9d6e9a79a683e660e572ab8e92f16",
        )
    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/74b146dc87d37baa1919da1e8f7b8aafbd32acd9.zip",
            strip_prefix = "rules_foreign_cc-74b146dc87d37baa1919da1e8f7b8aafbd32acd9",
            sha256 = "2de65ab702ebd0094da3885aae2a6a370df5edb4c9d0186096de79dffb356dbc",
        )

    # Transitive dependency of io_bazel_rules_docker
    if "bazel_skylib" not in native.existing_rules():
        skylib_version = "0.8.0"
        http_archive(
            name = "bazel_skylib",
            type = "tar.gz",
            url = "https://github.com/bazelbuild/bazel-skylib/releases/download/{}/bazel-skylib.{}.tar.gz".format(skylib_version, skylib_version),
            sha256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e",
        )
    if "bazel_gazelle" not in native.existing_rules():
        http_archive(
            name = "bazel_gazelle",
            sha256 = "3c681998538231a2d24d0c07ed5a7658cb72bfb5fd4bf9911157c0e9ac6a2687",
            urls = ["https://github.com/bazelbuild/bazel-gazelle/releases/download/0.17.0/bazel-gazelle-0.17.0.tar.gz"],
        )
    if "io_bazel_rules_go" not in native.existing_rules():
        http_archive(
            name = "io_bazel_rules_go",
            sha256 = "f04d2373bcaf8aa09bccb08a98a57e721306c8f6043a2a0ee610fd6853dcde3d",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/0.18.6/rules_go-0.18.6.tar.gz",
                "https://github.com/bazelbuild/rules_go/releases/download/0.18.6/rules_go-0.18.6.tar.gz",
            ],
        )

    # New dependencies.
    if "mpc_utils" not in native.existing_rules():
        http_archive(
            name = "mpc_utils",
            sha256 = "fc1ce0c27fb2450d164d8a4aaf562277f8f91b3eef1455f7bf1d0be55351f86e",
            url = "https://github.com/schoppmp/mpc-utils/archive/0c2be1bad53e3b7adb711d81d85d0c25549fad86.zip",
            strip_prefix = "mpc-utils-0c2be1bad53e3b7adb711d81d85d0c25549fad86",
        )

    if "io_bazel_rules_docker" not in native.existing_rules():
        http_archive(
            name = "io_bazel_rules_docker",
            sha256 = "87fc6a2b128147a0a3039a2fd0b53cc1f2ed5adb8716f50756544a572999ae9a",
            strip_prefix = "rules_docker-0.8.1",
            urls = ["https://github.com/bazelbuild/rules_docker/archive/v0.8.1.tar.gz"],
        )
