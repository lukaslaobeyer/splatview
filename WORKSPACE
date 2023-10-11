load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//deps/rules_pkg_config:pkg_config.bzl", "pkg_config_repository")

# Eigen
http_archive(
    name = "eigen",
    build_file = "@//deps/eigen:BUILD",
    sha256 = "8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72",
    url = "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz",
    strip_prefix="eigen-3.4.0"
)

# cxxopts
http_archive(
    name = "cxxopts",
    sha256 = "523175f792eb0ff04f9e653c90746c12655f10cb70f1d5e6d6d9491420298a08",
    url = "https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.1.1.tar.gz",
    strip_prefix = "cxxopts-3.1.1",
    build_file = "@//deps/cxxopts:BUILD",
)

# imgui
http_archive(
    name = "imgui",
    sha256 = "1acc27a778b71d859878121a3f7b287cd81c29d720893d2b2bf74455bf9d52d6",
    url = "https://github.com/ocornut/imgui/archive/refs/tags/v1.89.9.tar.gz",
    strip_prefix = "imgui-1.89.9",
    build_file = "@//deps/imgui:BUILD",
)

# quickcpplib (for llfio)
http_archive(
    name = "quickcpplib",
    sha256 = "88842a8f589ffbc9abed16bbc2cd360110811974b5879df7c3f5266365337747",
    url = "https://github.com/ned14/quickcpplib/archive/9aaddae850e2c6dd8a66a9833d1172d1515e9dbf.tar.gz",
    strip_prefix = "quickcpplib-9aaddae850e2c6dd8a66a9833d1172d1515e9dbf",
    build_file = "@//deps/llfio:quickcpplib.BUILD",
)

# outcome (for llfio)
http_archive(
    name = "outcome",
    sha256 = "553fd03bb9684be19dfa251bfa0064e69e30a95b6b0ba9a62d68f8ec4e31662a",
    url = "https://github.com/ned14/outcome/archive/refs/tags/v2.2.7.tar.gz",
    strip_prefix = "outcome-2.2.7",
    build_file = "@//deps/llfio:outcome.BUILD",
)

# llfio
http_archive(
    name = "llfio",
    sha256 = "ee1d51675309ac14fbc52438475835a6e40ef6a2fa8685798237565f47c46195",
    url = "https://github.com/ned14/llfio/archive/refs/tags/20230818.tar.gz",
    strip_prefix = "llfio-20230818",
    build_file = "@//deps/llfio:BUILD",
)

# GLAD
local_repository(
    name = "glad",
    path = "third_party/glad",
)

# GLFW
pkg_config_repository(
    name = "glfw",
    modname = "glfw3",
)
