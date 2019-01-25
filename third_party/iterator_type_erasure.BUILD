licenses = ["notice"]

cc_library(
    name = "any_iterator",
    hdrs = [
        "any_iterator/any_iterator.hpp",
        "any_iterator/detail/any_iterator_abstract_base.hpp",
        "any_iterator/detail/any_iterator_metafunctions.hpp",
        "any_iterator/detail/any_iterator_wrapper.hpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@boost//:conversion",
        "@boost//:iterator",
        "@boost//:type_traits",
        "@boost//:mpl",
    ],
)
