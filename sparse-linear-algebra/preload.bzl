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
            url = "https://github.com/nelhage/rules_boost/archive/6d6fd834281cb8f8e758dd9ad76df86304bf1869.zip",
            sha256 = "b4d498a21e9d90ec65720ee3ae4dcbc2f1ce245b2866242514cebbc189d2fc14",
            strip_prefix = "rules_boost-6d6fd834281cb8f8e758dd9ad76df86304bf1869",
        )

    if "com_github_schoppmp_rules_oblivc" not in native.existing_rules():
        http_archive(
            name = "com_github_schoppmp_rules_oblivc",
            sha256 = "188d699de79119d5b187c2f0be5316e036b26076fdbfa5d97d5ca11854a03c0a",
            url = "https://github.com/schoppmp/rules_oblivc/archive/67ff1f66696f1e4f59681128a5dfe7eec112ec36.zip",
            strip_prefix = "rules_oblivc-67ff1f66696f1e4f59681128a5dfe7eec112ec36",
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/99ea1b09fc3fe9a1fbb32d965324d9dc34c31cde.zip",
            strip_prefix = "rules_foreign_cc-99ea1b09fc3fe9a1fbb32d965324d9dc34c31cde",
            sha256 = "5ffb7001ae6fe455a1b7baceb0944112183d3523efcdc0a674a964837d4e7312",
        )

    # New dependencies.
    if "mpc_utils" not in native.existing_rules():
        http_archive(
            name = "mpc_utils",
            url = "https://github.com/schoppmp/mpc-utils/archive/defefb21bd4aa95a261ed6fc8fcf54c375b33012.zip",
            sha256 = "f7eca0fa10c4433f523872e4088f8a909246f8c7d661c41b81615179c5a8da48",
            strip_prefix = "mpc-utils-defefb21bd4aa95a261ed6fc8fcf54c375b33012",
        )
