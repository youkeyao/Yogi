#pragma once

#include "runtime/renderer/shader.h"
#include <glad/glad.h>

namespace Yogi {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
        ~OpenGLShader();

        void bind() const override;
        void unbind() const override;

        const std::string& get_name() const override {return m_name;}
    private:
        std::vector<uint32_t> read_file(const std::string& filepath);
    private:
        uint32_t m_renderer_id;
        std::string m_name;
    };

}