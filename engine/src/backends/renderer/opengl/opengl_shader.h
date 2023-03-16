#pragma once

#include "runtime/renderer/shader.h"
#include <glad/glad.h>
#include <spirv_glsl.hpp>

namespace Yogi {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
        ~OpenGLShader();

        void bind() const override;
        void unbind() const override;
    private:
        std::vector<uint32_t> read_file(const std::string& filepath);
        void reflect_vertex(const spirv_cross::CompilerGLSL& compiler);
        void reflect_uniform_buffer(const spirv_cross::CompilerGLSL& compiler);
    private:
        uint32_t m_renderer_id;
    };

}