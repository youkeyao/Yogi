#pragma once

#include "runtime/renderer/pipeline.h"
#include <glad/glad.h>
#include <spirv_glsl.hpp>

namespace Yogi {

    class OpenGLPipeline : public Pipeline
    {
    public:
        OpenGLPipeline(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
        ~OpenGLPipeline();

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