#include "dataset.h"
#include "logging.h"
#include "ply.h"
#include "tracing.h"

#include <cmath>

namespace viewer::dataset {

namespace {

void sort_fast(const SplatBuffer& buf, const Eigen::Matrix4f& P, SortResult* out) {
    // From https://github.com/antimatter15/splat
    const size_t N = buf.size();
    float min_d = std::numeric_limits<float>::infinity();
    float max_d = -std::numeric_limits<float>::infinity();

    {
        tracing::RecorderGuard tracing_guard("depth computation");
        constexpr int DEPTH_SCALE = 4096;
        for (size_t i = 0; i < N; ++i) {
            const float depth = DEPTH_SCALE * P.block<1, 3>(2, 0).dot(
                Eigen::Vector3f(buf.at(i).center[0],
                                buf.at(i).center[1],
                                buf.at(i).center[2]));
            out->sizes[i] = static_cast<int>(depth);
            max_d = std::max(depth, max_d);
            min_d = std::min(depth, min_d);
        }
    }

    {
        tracing::RecorderGuard tracing_guard("counting sort");
        const int32_t M = static_cast<int32_t>(out->counts0.size());
        const float depth_inv = M / (max_d - min_d);
        for (size_t i = 0; i < N; ++i) {
            out->sizes[i] =
                std::clamp(static_cast<int32_t>((out->sizes[i] - min_d) * depth_inv), 0, M - 1);
            ++(out->counts0[out->sizes[i]]);
        }
        for (size_t i = 1; i < static_cast<size_t>(M); ++i)
            out->starts0[i] = out->starts0[i - 1] + out->counts0[i - 1];
        for (size_t i = 0; i < N; ++i)
            out->depth_index[out->starts0[out->sizes[i]]++] = i;
    }
}

void sort_std(const SplatBuffer& buf, const Eigen::Matrix4f& P, SortResult* out) {
    const size_t N = buf.size();

    {
        tracing::RecorderGuard tracing_guard("depth computation");
        for (size_t i = 0; i < N; ++i) {
            out->depths[i] = P.block<1, 3>(2, 0).dot(
                Eigen::Vector3f(buf.at(i).center[0],
                                buf.at(i).center[1],
                                buf.at(i).center[2]));
        }
    }

    {
        tracing::RecorderGuard tracing_guard("std::sort");
        for (size_t i = 0; i < N; ++i)
            out->depth_index[i] = i;
        std::sort(out->depth_index.begin(), out->depth_index.end(),
                  [&](uint32_t i, uint32_t j) {
                      return out->depths[i] < out->depths[j];
                  });
        tracing_guard.print();
    }
}

}

Dataset from_ply(const std::string& filename) {
    tracing::RecorderGuard tracing_guard("load dataset");
    ply::PlyFile ply(filename);

    // Create accessors
    const auto x = ply.accessor<float>("x");
    const auto y = ply.accessor<float>("y");
    const auto z = ply.accessor<float>("z");
    const auto opacity = ply.accessor<float>("opacity");
    const auto scale_0 = ply.accessor<float>("scale_0");
    const auto scale_1 = ply.accessor<float>("scale_1");
    const auto scale_2 = ply.accessor<float>("scale_2");
    const auto rot_qw = ply.accessor<float>("rot_0");
    const auto rot_qx = ply.accessor<float>("rot_1");
    const auto rot_qy = ply.accessor<float>("rot_2");
    const auto rot_qz = ply.accessor<float>("rot_3");

    // Spherical harmonics accessors
    const auto f_dc_0 = ply.accessor<float>("f_dc_0");
    const auto f_dc_1 = ply.accessor<float>("f_dc_1");
    const auto f_dc_2 = ply.accessor<float>("f_dc_2");
    std::vector<ply::PlyAccessor<float>> sh;
    for (size_t i = 0; i < 45; ++i)
        sh.push_back(ply.accessor<float>("f_rest_" + std::to_string(i)));

    SplatBuffer buffer(ply.num_vertices());
    {
        tracing::RecorderGuard tracing_guard("buffer population");
        for (size_t row = 0; row < ply.num_vertices(); ++row) {
            Splat& splat = buffer.at(row);

            // Mean of each Gaussian
            splat.center[0] = x(row);
            splat.center[1] = y(row);
            splat.center[2] = z(row);

            // Covariance
            const Eigen::DiagonalMatrix<float, 3> scale(
                std::exp(scale_0(row)),
                std::exp(scale_1(row)),
                std::exp(scale_2(row)));
            const Eigen::Vector4f quat_coeffs(
                rot_qx(row),
                rot_qy(row),
                rot_qz(row),
                rot_qw(row));
            const Eigen::Matrix3f R(
                Eigen::Quaternionf(quat_coeffs.normalized()));
            const Eigen::Matrix3f M = R * scale;
            splat.covA[0] = M.row(0).dot(M.row(0));
            splat.covA[1] = M.row(0).dot(M.row(1));
            splat.covA[2] = M.row(0).dot(M.row(2));
            splat.covB[0] = M.row(1).dot(M.row(1));
            splat.covB[1] = M.row(1).dot(M.row(2));
            splat.covB[2] = M.row(2).dot(M.row(2));

            // Alpha
            splat.alpha = 1.f / (1.f + std::exp(-opacity(row)));

            // Color (spherical harmonics)
            splat.sh[0][0] = f_dc_0(row);
            splat.sh[0][1] = f_dc_1(row);
            splat.sh[0][2] = f_dc_2(row);
            for (size_t sh_idx = 1; sh_idx < 15; ++sh_idx) {
                splat.sh[sh_idx][0] = sh.at(sh_idx - 1)(row);
                splat.sh[sh_idx][1] = sh.at(sh_idx + 14)(row);
                splat.sh[sh_idx][2] = sh.at(sh_idx + 29)(row);
            }

        }
        tracing_guard.print();
    }

    return Dataset(std::move(buffer));
}

void Dataset::sort(const Eigen::Matrix4f& P, SortResult* out, bool fast_sort) const {
    tracing::RecorderGuard tracing_guard("sort");

    const size_t N = buffer_.size();
    out->reset(N);

    if (fast_sort)
        sort_fast(buffer_, P, out);
    else
        sort_std(buffer_, P, out);
}

}
