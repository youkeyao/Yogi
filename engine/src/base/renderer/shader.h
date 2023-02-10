#pragma once

#include <glm/glm.hpp>

namespace Yogi {

    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual const std::string& get_name() const = 0;

        virtual void set_int(const std::string& name, int value) const = 0;
        virtual void set_int_array(const std::string& name, int* values, uint32_t count) const = 0;
        virtual void set_float(const std::string& name, float value) const = 0;
        virtual void set_float3(const std::string& name, const glm::vec3& value) const = 0;
        virtual void set_float4(const std::string& name, const glm::vec4& value) const = 0;
        virtual void set_mat4(const std::string& name, const glm::mat4& value) const = 0;

        static Ref<Shader> create(const std::string& filepath);
        static Ref<Shader> create(const std::string& name, const std::string& vertex_source, const std::string& fragment_source);
    private:
        uint32_t m_renderer_id;
    };

}