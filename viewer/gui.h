#pragma once

#include "render.h"

#include <Eigen/Dense>
#include <GLFW/glfw3.h>

namespace viewer::gui {

class Gui {
public:
    Gui(GLFWwindow* window) : window_(window) {}

    void render();

    bool close_requested = false;

    // Camera controls
    float fov_deg = 60.f;
    float rx, ry, rz;
    float px, py, pz;

    // Rendering controls
    bool enable_vsync;
    rendering::RendererConfig renderer_config;

    Eigen::Matrix4f mat_view = Eigen::Matrix4f::Identity();
    Eigen::Vector3f cam_ypr = Eigen::Vector3f::Zero();
    Eigen::Vector3f cam_position = Eigen::Vector3f::Zero();
private:
    GLFWwindow* window_;
};

}
