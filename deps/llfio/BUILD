load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "llfio",
    hdrs = [ "include/llfio.hpp" ],
    includes = [ "include" ],
    visibility = ["//visibility:public"],
    deps = [ ":llfio_impl" ],
)

cc_library(
    name = "llfio_impl",
    hdrs = glob([
        "include/llfio/**/*.h",
        "include/llfio/**/*.hpp",
        "include/llfio/**/*.ixx",
        "include/llfio/**/*.ipp",
        "include/kvstore/**/*.hpp",
    ]),
    includes = [ "include" ],
    deps = [ "@quickcpplib", "@outcome" ],
)
