#pragma once

#include <glad/glad.h>

#include <string_view>
#include <string>
#include <expected>
#include <array>

namespace yahbog {

    class compute_shader {
    public:
        using make_return_t = std::expected<compute_shader, std::string>;

        static make_return_t make(std::string_view source) {
            auto shader = compute_shader{};
            auto id = glCreateShader(GL_COMPUTE_SHADER);
            if(id == 0) {
                return std::unexpected{std::string{"Failed to create compute shader"}};
            }

            auto source_cstr = source.data();
            glShaderSource(id, 1, &source_cstr, nullptr);
            glCompileShader(id);

            GLint status;
            glGetShaderiv(id, GL_COMPILE_STATUS, &status);
            if(status == GL_FALSE) {
                GLint info_log_length;
                glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
                std::string info_log(info_log_length, '\0');
                glGetShaderInfoLog(id, info_log_length, nullptr, info_log.data());
                glDeleteShader(id);
                return std::unexpected{std::move(info_log)};
            }

            shader.id = glCreateProgram();
            glAttachShader(shader.id, id);
            glLinkProgram(shader.id);

            GLint link_status;
            glGetProgramiv(shader.id, GL_LINK_STATUS, &link_status);
            if(link_status == GL_FALSE) {
                GLint info_log_length;
                glGetProgramiv(shader.id, GL_INFO_LOG_LENGTH, &info_log_length);
                std::string info_log(info_log_length, '\0');
                glGetProgramInfoLog(shader.id, info_log_length, nullptr, info_log.data());
                glDeleteProgram(shader.id);
                return std::unexpected{std::move(info_log)};
            }

            glDeleteShader(id);
            return shader;
        }

        void use() const {
            glUseProgram(id);
        }

        void dispatch(GLuint x, GLuint y) {
            glDispatchCompute(x, y, 1);
        }

        void uniform(std::string_view name, bool value) const {
            GLint location = glGetUniformLocation(id, name.data());
            glUniform1i(location, value);
        }

        void uniform(std::string_view name, int value) const {
            GLint location = glGetUniformLocation(id, name.data());
            glUniform1i(location, value);
        }

        void uniform(std::string_view name, float value) const {
            GLint location = glGetUniformLocation(id, name.data());
            glUniform1f(location, value);
        }

        template<std::size_t N>
        void uniform(std::string_view name, const std::array<float, N>& value) const {
            GLint location = glGetUniformLocation(id, name.data());
            glUniform1fv(location, N, value.data());
        }

        template<std::size_t N>
        void uniform(std::string_view name, const std::array<std::array<float, 4>, N>& value) const {
            GLint location = glGetUniformLocation(id, name.data());
            glUniform4fv(location, N, reinterpret_cast<const GLfloat*>(value.data()));
        }

        constexpr compute_shader(compute_shader&& other) noexcept {
            std::swap(id, other.id);
        }

        constexpr compute_shader& operator=(compute_shader&& other) noexcept {
            std::swap(id, other.id);
            return *this;
        }

        compute_shader(const compute_shader&) = delete;
        compute_shader& operator=(const compute_shader&) = delete;

        constexpr compute_shader() : id(0) {}

        constexpr explicit operator bool() const {
            return id != 0;
        }

        ~compute_shader() {
            if(id != 0) {
                glDeleteProgram(id);
            }
        }

    private:
        GLuint id;
    };

}