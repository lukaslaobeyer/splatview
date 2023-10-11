#pragma once

#include "dataset.h"

#include <thread>
#include <mutex>

namespace viewer::rendering {
    struct CameraIntrinsics {
        float fx;
        float fy;
        float width;
        float height;
    };

    struct RendererConfig {
        bool use_fast_sort = true;
        int sh_degree = 3;
    };
    
    class Renderer {
    public:
        Renderer(const dataset::Dataset& d);
        void use_program() const;
        void set_camera_intrinsics(const CameraIntrinsics& c);
        void set_view(const Eigen::Matrix4f& view);
        void set_config(const RendererConfig& config);
        void render() const;
    private:
        void sort_worker(std::stop_token stop);
    private:
        const dataset::Dataset& d_;
        
        uint32_t program_;
        int32_t u_projection_;
        int32_t u_viewport_;
        int32_t u_focal_;
        int32_t u_view_;
        int32_t u_cam_pos_;
        int32_t u_sh_degree_;

        std::array<float, 8> triangle_vertices_;

        uint32_t ssbo_splats_;
        uint32_t buf_vertex_;
        uint32_t buf_index_;

        Eigen::Matrix4f mat_projection_;
        Eigen::Matrix4f mat_view_;
        RendererConfig config_;

        mutable size_t prev_buffer_index_;
        mutable size_t buffer_index_;
        mutable dataset::SortResult sr0_;
        mutable dataset::SortResult sr1_;

        mutable std::mutex mutex_;
        std::jthread thread_;
    };
}
