load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "outcome",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
        "include/**/*.ixx",
        "include/**/*.ipp",
    ]),
    includes = [ "include" ],
    visibility = ["//visibility:public"],
    deps = [ "@quickcpplib" ],
)
