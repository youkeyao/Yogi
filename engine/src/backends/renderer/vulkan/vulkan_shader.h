#pragma once

#include "runtime/renderer/shader.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
        ~VulkanShader();

        void bind() const override;
        void unbind() const override;

        const std::string& get_name() const override {return m_name;}
    private:
        std::vector<uint8_t> read_file(const std::string& filepath);
    private:
        uint32_t m_renderer_id;
        std::string m_name;
    };

}