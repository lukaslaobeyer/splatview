load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "imgui",
    srcs = [
        "imgui.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "backends/imgui_impl_glfw.cpp",
        "backends/imgui_impl_opengl3.cpp",	
    ],
    hdrs = glob([
        "*.h",
        "backends/*.h",
    ]),
    defines = [ "GLFW_INCLUDE_NONE" ],
    include_prefix = "imgui",
    visibility = ["//visibility:public"],
    deps = [
        "@glfw",
    ]
)
