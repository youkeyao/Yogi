#pragma once

#include "runtime/renderer/shader.h"
#include <vulkan/vulkan.h>
#include <spirv_glsl.hpp>

namespace Yogi {

    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" });
        ~VulkanShader();

        void bind() const override;
        void unbind() const override;

        const std::vector<VkDescriptorSet>& get_descriptor_sets() { return m_descriptor_sets; }
        VkPipeline get_vk_pipeline() { return m_graphics_pipeline; }
    private:
        std::vector<uint32_t> read_file(const std::string& filepath);
        VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
        void reflect_vertex(const spirv_cross::CompilerGLSL& compiler, VkVertexInputBindingDescription& binding_description, std::vector<VkVertexInputAttributeDescription>& attribute_descriptions);
        void reflect_uniform_buffer(const spirv_cross::CompilerGLSL& compiler, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& ubo_layout_bindings, VkShaderStageFlagBits stage_flag);
    private:
        std::string m_name;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_graphics_pipeline;
        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        std::vector<VkDescriptorSet> m_descriptor_sets;
        VkDescriptorPool m_descriptor_pool;
    };

}