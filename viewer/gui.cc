#include "gui.h"
#include "logging.h"
#include <imgui/imgui.h>
#include <cmath>
#include <iostream>

namespace viewer::gui {

namespace {

Eigen::Vector2f to_eigen(const ImVec2& v) {
    return Eigen::Vector2f(v.x, v.y);
}

template <typename EigenDerivedT>
Eigen::Matrix4f translation(const EigenDerivedT& t) {
    Eigen::Matrix4f T;
    T.setIdentity();
    T.block<3, 1>(0, 3) = t.template head<3>();
    return T;
}

Eigen::Matrix4f rotation(const Eigen::Quaternionf& q) {
    Eigen::Matrix4f R;
    R.setIdentity();
    R.block<3, 3>(0, 0) = q.toRotationMatrix();
    return R;
}

Eigen::Matrix4f rotation_from_euler(float yaw, float pitch, float roll) {
    return rotation(
        Eigen::Quaternionf(
            Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitZ())
            * Eigen::AngleAxisf(pitch, Eigen::Vector3f::UnitX())
            * Eigen::AngleAxisf(yaw, Eigen::Vector3f::UnitY())));
}

void handle_rotation(Eigen::Vector3f* cam_ypr) {
    static Eigen::Vector2f start_pos;

    if (ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow)) return;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;

    const Eigen::Vector2f display_size(to_eigen(ImGui::GetIO().DisplaySize));
    const Eigen::Vector2f curr_pos(to_eigen(ImGui::GetMousePos()));

    if (ImGui::IsMouseDragging(0)) {
        const auto delta = (curr_pos - start_pos).cwiseQuotient(display_size);
        (*cam_ypr)(0) += delta(0);
        (*cam_ypr)(1) += -delta(1);
    }

    start_pos = curr_pos;
}

void handle_translation(
    float dt, Eigen::Matrix4f mat_view,
    float speed, Eigen::Vector3f* cam_position) {
    const int forward =
        ImGui::IsKeyDown(ImGuiKey_W) - ImGui::IsKeyDown(ImGuiKey_S);
    const int left =
        ImGui::IsKeyDown(ImGuiKey_A) - ImGui::IsKeyDown(ImGuiKey_D);
    const int up =
        ImGui::IsKeyDown(ImGuiKey_E) - ImGui::IsKeyDown(ImGuiKey_Q);
    if (forward != 0 || left != 0 || up != 0) {
        const Eigen::Vector4f step =
            mat_view.inverse()
            * Eigen::Vector4f(left, up, -forward, 0.f).normalized()
            * speed * dt;
        *cam_position += step.head<3>();
    }
}

}

void Gui::render() {
    const float dt = ImGui::GetIO().DeltaTime;

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        close_requested = true;

    ImGui::SeparatorText("Stats");

    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

    ImGui::SeparatorText("Camera");

    ImGui::SliderFloat("FOV", &fov_deg, 0.f, 180.f, "FOV = %.2f");

    static float pullback = 0.f;
    ImGui::DragFloat("pullback", &pullback, 0.05f, 0.f);

    static float kb_control_speed = 1.f;
    ImGui::DragFloat("keyboard controls speed", &kb_control_speed, 0.05f, 0.f);

    {
        handle_rotation(&cam_ypr);
        handle_translation(dt, mat_view, kb_control_speed, &cam_position);
        mat_view =
            translation(pullback * Eigen::Vector3f::UnitZ())
            * rotation_from_euler(cam_ypr(0), cam_ypr(1), cam_ypr(2))
            * translation(-pullback * Eigen::Vector3f::UnitZ())
            * translation(cam_position);
    }

    {
        ImGui::DragFloat3("camera position", cam_position.data(), 0.1f);
        ImGui::DragFloat3("camera yaw/pitch/roll", cam_ypr.data(), 0.01f);
    }

    ImGui::SeparatorText("Renderer");
    ImGui::Checkbox("enable vsync", &enable_vsync);
    ImGui::Checkbox("use fast sorting algorithm", &renderer_config.use_fast_sort);
    ImGui::SliderInt("spherical harmonics degree", &renderer_config.sh_degree, 0, 3);
}

}
