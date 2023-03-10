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

        static Ref<Shader> create(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
    private:
        uint32_t m_renderer_id;
    };

}