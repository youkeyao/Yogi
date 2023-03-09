#pragma once

#include "runtime/renderer/shader.h"
#include <glm/glm.hpp>
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

        void set_int(const std::string& name, int value) const override;
        void set_int_array(const std::string& name, int* values, uint32_t count) const override;
        void set_float(const std::string& name, float value) const override;
        void set_float3(const std::string& name, const glm::vec3& value) const override;
        void set_float4(const std::string& name, const glm::vec4& value) const override;
        void set_mat4(const std::string& name, const glm::mat4& value) const override;
    private:
        std::vector<uint8_t> read_file(const std::string& filepath);
    private:
        uint32_t m_renderer_id;
        std::string m_name;
    };

}