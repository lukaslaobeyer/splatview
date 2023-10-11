#include "logging.h"
#include "dataset.h"
#include "render.h"
#include "gui.h"

#include <iostream>
#include <cxxopts.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

namespace viewer {

void glfw_error_callback(int error, const char* description) {
    LOG_ERROR("GLFW: errno %d, %s", error, description);
}

void gl_error_callback(GLenum /* source */,
                       GLenum /* type */,
                       GLuint /* id */,
                       GLenum /* severity */,
                       GLsizei length,
                       const GLchar* message,
                       const void* /* user_param */) {
    LOG_INFO("GL debug: %.*s", length, message);
}

void render(const rendering::Renderer& r, gui::Gui& gui) {
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    r.render();
    gui.render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void init_imgui(GLFWwindow* window)
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Setup style
    ImGui::StyleColorsDark();
}

}

int main(int argc, char** argv) {
    using namespace viewer;

    cxxopts::Options options(argv[0], "3D Gaussian Splat Viewer");
    // clang-format off
    options
        .positional_help("file.ply")
        .add_options()
            ("h,help", "print this help message")
            ("disable-vsync", "disable vsync")
            ("gl-debug", "print OpenGL debug messages")
            ("positional", "", cxxopts::value<std::vector<std::string>>());
    // clang-format on

    options.parse_positional({"positional"});
    auto parsed_options = options.parse(argc, argv);

    bool enable_vsync = parsed_options.count("disable-vsync") == 0;
    const bool enable_gldebug = parsed_options.count("gl-debug") == 1;

    if (parsed_options.count("help") || parsed_options.count("positional") == 0 ||
        parsed_options["positional"].as<std::vector<std::string>>().size() != 1) {
        std::cout << options.help() << std::endl;
        return -1;
    }

    const std::string ply_file_name =
        parsed_options["positional"].as<std::vector<std::string>>().at(0);
    LOG_INFO("loading %s...", ply_file_name.c_str());
    const dataset::Dataset d(dataset::from_ply(ply_file_name));
    LOG_INFO("done");

    if (!glfwInit()) {
        LOG_ERROR("GLFW init failed");
        return -1;
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (enable_gldebug)
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, argv[0], NULL, NULL);
    if (!window) {
        LOG_ERROR("GLFW window creation failed");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_ERROR("GLAD init failed");
        glfwTerminate();
        return -1;
    }

    glDebugMessageCallback(gl_error_callback, 0);
    init_imgui(window);

    rendering::Renderer renderer(d);
    gui::Gui gui(window);

    glfwSwapInterval(enable_vsync ? 1 : 0);
    gui.enable_vsync = enable_vsync;

    int width, height;
    while (!glfwWindowShouldClose(window) && !gui.close_requested) {
        if (gui.enable_vsync != enable_vsync) {
            enable_vsync = gui.enable_vsync;
            glfwSwapInterval(enable_vsync ? 1 : 0);
        }
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        renderer.use_program();
        const float fxy = 0.5f * width / std::tan(0.5f * gui.fov_deg * M_PI / 180.f);
        renderer.set_camera_intrinsics(
            {.fx = fxy, .fy = fxy,
             .width = static_cast<float>(width),
             .height = static_cast<float>(height)});
        renderer.set_config(gui.renderer_config);
        renderer.set_view(gui.mat_view);
        render(renderer, gui);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
