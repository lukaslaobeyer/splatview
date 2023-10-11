load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "quickcpplib",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
        "include/**/*.ixx",
    ]),
    includes = [ "include" ],
    visibility = ["//visibility:public"],
)
