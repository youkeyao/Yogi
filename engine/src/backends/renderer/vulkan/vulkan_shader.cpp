#include "backends/renderer/vulkan/vulkan_shader.h"

namespace Yogi {

    Ref<Shader> Shader::create(const std::string& name, const std::vector<std::string>& types)
    {
        return CreateRef<VulkanShader>(name);
    }

    VulkanShader::VulkanShader(const std::string& name, const std::vector<std::string>& types) : m_name(name)
    {
        YG_PROFILE_FUNCTION();
    }

    VulkanShader::~VulkanShader()
    {
        YG_PROFILE_FUNCTION();
    }

    std::vector<uint8_t> VulkanShader::read_file(const std::string& filepath)
    {
        std::vector<uint8_t> buffer;
        std::ifstream in(filepath, std::ios::ate | std::ios::binary);

        if (!in.is_open()) {
            YG_CORE_ERROR("Could not open file '{0}'!", filepath);
            return buffer;
        }

        in.seekg(0, std::ios::end);
        buffer.resize(in.tellg() / 4);
        in.seekg(0, std::ios::beg);
        in.read((char*)buffer.data(), buffer.size() * 4);
        in.close();

        return buffer;
    }

    void VulkanShader::bind() const
    {
        YG_PROFILE_FUNCTION();
    }

    void VulkanShader::unbind() const
    {
        YG_PROFILE_FUNCTION();
    }

}