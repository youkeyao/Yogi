#pragma once

#include <glm/glm.hpp>

namespace hazel {

    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual void set_int(const std::string& name, int) const = 0;
        virtual void set_float3(const std::string& name, const glm::vec3&) const = 0;
        virtual void set_float4(const std::string& name, const glm::vec4&) const = 0;
        virtual void set_mat4(const std::string& name, const glm::mat4&) const = 0;

        static Shader* create(const std::string& filepath);
        static Shader* create(const std::string& vertex_source, const std::string& fragment_source);
    private:
        uint32_t m_renderer_id;
    };

}