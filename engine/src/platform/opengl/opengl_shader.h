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

        void set_int(const std::string& name, int) const override;
        void set_float3(const std::string& name, const glm::vec3&) const override;
        void set_float4(const std::string& name, const glm::vec4&) const override;
        void set_mat4(const std::string& name, const glm::mat4&) const override;
    private:
        std::string read_file(const std::string& filepath);
        std::unordered_map<GLenum, std::string> preprocess(const std::string& source);
        void compile(const std::unordered_map<GLenum, std::string>& sources);
    private:
        uint32_t m_renderer_id;
        std::string m_name;
    };

}