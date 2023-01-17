#pragma once

#include "renderer/shader.h"
#include <glm/glm.hpp>

typedef unsigned int GLenum;

namespace hazel {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& filepath);
        OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~OpenGLShader();

        void bind() const override;
        void unbind() const override;

        const std::string& get_name() const override {return m_name;}

        void set_int(const std::string& name, int value) const override;
        void set_float(const std::string& name, float value) const override;
        void set_float3(const std::string& name, const glm::vec3& value) const override;
        void set_float4(const std::string& name, const glm::vec4& value) const override;
        void set_mat4(const std::string& name, const glm::mat4& value) const override;
    private:
        std::string read_file(const std::string& filepath);
        std::unordered_map<GLenum, std::string> preprocess(const std::string& source);
        void compile(const std::unordered_map<GLenum, std::string>& sources);
    private:
        uint32_t m_renderer_id;
        std::string m_name;
    };

}