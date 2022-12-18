#pragma once

#include "renderer/shader.h"
#include <glm/glm.hpp>

namespace hazel {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~OpenGLShader();

        void bind() const override;
        void unbind() const override;

        void set_int(const std::string& name, int) const override;
        void set_float3(const std::string& name, const glm::vec3&) const override;
        void set_float4(const std::string& name, const glm::vec4&) const override;
        void set_mat4(const std::string& name, const glm::mat4&) const override;
    private:
        uint32_t m_renderer_id;
    };

}