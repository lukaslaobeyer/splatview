cc_binary(
    name = "viewer",
    srcs = [
        "viewer.cc"
    ],
    defines = [ "GLFW_INCLUDE_NONE" ],
    deps = [
    	":dataset",
        ":gui",
        ":render",
        ":logging",
	"@cxxopts",
	"@imgui",
	"@glad",
	"@glfw",
    ],
)

cc_library(
    name = "render",
    srcs = ["render.cc"],
    hdrs = ["render.h"],
    textual_hdrs = [
        "shaders/shader.vs",
        "shaders/shader.fs",
    ],
    defines = [ "GLFW_INCLUDE_NONE" ],
    deps = [
        ":logging",
	":tracing",
	":dataset",
	"@glad",
	"@glfw",
    ]
)

cc_library(
    name = "dataset",
    srcs = ["dataset.cc"],
    hdrs = ["dataset.h"],
    deps = [
        ":logging",
	":ply",
	":tracing",
        "@eigen",
    ]
)

cc_library(
    name = "ply",
    hdrs = ["ply.h"],
    srcs = ["ply.cc"],
    defines = [
        # Does not compile without this define...
        "LLFIO_DISABLE_SIGNAL_GUARD"
    ],
    deps = [
        ":logging",
        "@llfio",
    ]
)

cc_library(
    name = "gui",
    srcs = ["gui.cc"],
    hdrs = ["gui.h"],
    defines = [ "GLFW_INCLUDE_NONE" ],
    deps = [
        ":logging",
	":render",
	"@imgui",
	"@eigen",
    ]
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
)

cc_library(
    name = "tracing",
    hdrs = ["tracing.h"],
    deps = [
        ":logging",
    ]
)