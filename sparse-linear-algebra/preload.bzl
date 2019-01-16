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
            sha256 = "e985073252c21df2b890741c5ad402c6fc6965852336e7f2a07042f3d2797c48",
            url = "https://github.com/schoppmp/rules_oblivc/archive/8067bf3d918d25b001a853d6a1085cc21be6d6de.zip",
            strip_prefix = "rules_oblivc-8067bf3d918d25b001a853d6a1085cc21be6d6de",
        )

    if "rules_foreign_cc" not in native.existing_rules():
        http_archive(
            name = "rules_foreign_cc",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/99ea1b09fc3fe9a1fbb32d965324d9dc34c31cde.zip",
            strip_prefix = "rules_foreign_cc-99ea1b09fc3fe9a1fbb32d965324d9dc34c31cde",
            sha256 = "5ffb7001ae6fe455a1b7baceb0944112183d3523efcdc0a674a964837d4e7312",
        )

    # New dependencies.
    if "mpc-utils" not in native.existing_rules():
        http_archive(
            name = "com_github_schoppmp_mpc_utils",
            url = "https://github.com/schoppmp/mpc-utils/archive/49534f7136426ac96d3282372b2d6b12a4ce46e5.zip",
            sha256 = "b41db661c99289c102c4633956fb720ee3bd366bba6ea6a252adc9e50ab68b84",
            strip_prefix = "mpc-utils-49534f7136426ac96d3282372b2d6b12a4ce46e5",
        )