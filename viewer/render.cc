#include "render.h"
#include "tracing.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace viewer::rendering {

namespace {

static const char* VERTEX_SHADER_SOURCE =
#include "shaders/shader.vs"
;
static const char* FRAGMENT_SHADER_SOURCE =
#include "shaders/shader.fs"
;

uint32_t create_shaders() {
    constexpr GLsizei MAX_INFO_LOG_LENGTH = 2000;
    GLsizei info_log_length;
    GLchar info_log[MAX_INFO_LOG_LENGTH];
    GLint compilation_status;
    auto check_comp_status = [&](GLuint shader) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compilation_status);
        if (compilation_status == GL_TRUE)
            return;

        LOG_ERROR("shader compilation failure");
        glGetShaderInfoLog(shader, MAX_INFO_LOG_LENGTH, &info_log_length, info_log);
        if (info_log_length >= MAX_INFO_LOG_LENGTH)
            info_log[MAX_INFO_LOG_LENGTH - 1] = 0;
        LOG_INFO("error message:\n%s", info_log);
        LOG_FATAL("aborting");
    };

    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &VERTEX_SHADER_SOURCE, NULL);
    glCompileShader(vertex_shader);
    check_comp_status(vertex_shader);

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &FRAGMENT_SHADER_SOURCE, NULL);
    glCompileShader(fragment_shader);
    check_comp_status(fragment_shader);

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &compilation_status);
    if (compilation_status != GL_TRUE)
        LOG_FATAL("GL program link failure.");

    return program;
}

template <typename Container>
GLuint ssbo_setup(const Container& d) {
    const size_t data_num_bytes =
        sizeof(typename Container::value_type) * d.size();

    GLint max_size;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_size);
    if (max_size <= 0 || data_num_bytes > static_cast<size_t>(max_size))
        LOG_FATAL("ssbo size too large, %ld (requested) > %d (max size)",
                  data_num_bytes, max_size);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data_num_bytes,
                 d.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

bool is_integer_gl_type(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return true;
    default:
        return false;
    }
}

template <typename T>
GLuint buf_setup(
    GLenum type,
    GLuint program,
    const char* attrib_name,
    GLint vertex_size = 1,
    bool enable_instanced_rendering = true,
    T* vertex_data = nullptr,
    size_t num_vertex_data = 0) {
    glUseProgram(program);

    GLuint buffer;
    glGenBuffers(1, &buffer);
    if (vertex_data) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            num_vertex_data * sizeof(T),
            vertex_data,
            GL_STATIC_DRAW);
    }
    GLuint a = glGetAttribLocation(program, attrib_name);
    glEnableVertexAttribArray(a);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    if (is_integer_gl_type(type))
        glVertexAttribIPointer(a, vertex_size, type, 0, 0);
    else
        glVertexAttribPointer(a, vertex_size, type, false, 0, 0);
    if (enable_instanced_rendering)
        glVertexAttribDivisor(a, 1);

    return buffer;
}

template <typename Container>
void buf_data(GLuint buf, const Container& d, GLenum usage = GL_DYNAMIC_DRAW) {
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(typename Container::value_type) * d.size(),
                 d.data(),
                 usage);
}

}

Renderer::Renderer(const dataset::Dataset& d)
    : d_(d)
    , program_(create_shaders())
    , u_projection_(glGetUniformLocation(program_, "projection"))
    , u_viewport_(glGetUniformLocation(program_, "viewport"))
    , u_focal_(glGetUniformLocation(program_, "focal"))
    , u_view_(glGetUniformLocation(program_, "view"))
    , u_cam_pos_(glGetUniformLocation(program_, "cam_position"))
    , u_sh_degree_(glGetUniformLocation(program_, "sh_degree"))
    , triangle_vertices_({-2.f, -2.f, 2.f, -2.f, 2.f, 2.f, -2.f, 2.f})
      // Set up buffers:
    , ssbo_splats_(ssbo_setup(d.buffer()))
    , buf_vertex_(buf_setup(GL_FLOAT,
                            program_, "position", 2, false,
                            triangle_vertices_.data(),
                            triangle_vertices_.size()))
    , buf_index_(buf_setup<uint32_t>(GL_UNSIGNED_INT, program_, "depth_index"))
    , thread_(std::bind_front(&Renderer::sort_worker, this)) {
    glUseProgram(program_);
    // General setup
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(
        GL_ONE_MINUS_DST_ALPHA,
        GL_ONE,
        GL_ONE_MINUS_DST_ALPHA,
        GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
}

void Renderer::use_program() const {
    glUseProgram(program_);
}

void Renderer::set_camera_intrinsics(const CameraIntrinsics& c) {
    constexpr float z_near = 0.2f;
    constexpr float z_far = 200.f;
    constexpr float dz = z_far - z_near;
    {
        std::lock_guard lg(mutex_);
        // clang-format off
        mat_projection_ <<
            2.f * c.fx / c.width,  0.f,                   0.f,         0.f,
            0.f,                  -2.f * c.fy / c.height, 0.f,         0.f,
            0.f,                   0.f,                   z_far / dz, -z_far * z_near / dz,
            0.f,                   0.f,                   1.f,         0.f;
        // clang-format on
    }
    glUniformMatrix4fv(u_projection_, 1, GL_FALSE, mat_projection_.data());
    glUniform2f(u_viewport_, c.width, c.height);
    glUniform2f(u_focal_, c.fx, c.fy);
}

void Renderer::set_view(const Eigen::Matrix4f& view) {
    {
        std::lock_guard lg(mutex_);
        mat_view_ = view;
    }
    glUniformMatrix4fv(u_view_, 1, GL_FALSE, mat_view_.data());
    const Eigen::Vector3f cam_pos(mat_view_.inverse().block<3, 1>(0, 3));
    glUniform3fv(u_cam_pos_, 1, cam_pos.data());
}

void Renderer::set_config(const RendererConfig& config) {
    {
        std::lock_guard lg(mutex_);
        config_ = config;
    }
    glUniform1i(u_sh_degree_, config_.sh_degree);
}

void Renderer::render() const {
    tracing::RecorderGuard tracing_guard("render");
    use_program();
    {
        std::lock_guard lg(mutex_);

        const dataset::SortResult& sr = buffer_index_ == 0 ? sr0_ : sr1_;

        if (buffer_index_ != prev_buffer_index_) {
            buf_data(buf_index_, sr.depth_index);
            prev_buffer_index_ = buffer_index_;
        }

        {
            tracing::RecorderGuard tracing_guard("draw");
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_splats_);
            glDrawArraysInstanced(
                GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(sr.num_vertices()));
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
        }
    }
}

void Renderer::sort_worker(std::stop_token stop) {
    while (!stop.stop_requested()) {
        tracing::RecorderGuard tracing_guard("sort worker");
        Eigen::Matrix4f P;
        bool use_fast_sort;
        {
            std::lock_guard lg(mutex_);
            P = mat_projection_ * mat_view_;
            use_fast_sort = config_.use_fast_sort;
        }
        {
            tracing::RecorderGuard tracing_guard("sort");
            dataset::SortResult& sr = buffer_index_ == 0 ? sr1_ : sr0_;
            d_.sort(P, &sr, use_fast_sort);
        }
        {
            std::lock_guard lg(mutex_);
            buffer_index_ = (buffer_index_ + 1) % 2;
        }
        // tracing_guard.print();
    }
}

}
