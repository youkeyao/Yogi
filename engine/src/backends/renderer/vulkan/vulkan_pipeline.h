#pragma once

#include "runtime/renderer/pipeline.h"
#include <vulkan/vulkan.h>
#include <spirv_glsl.hpp>

namespace Yogi {

    class VulkanPipeline : public Pipeline
    {
    public:
        VulkanPipeline(const std::string& name, const std::vector<std::string>& types = { "vert", "frag" }, bool is_last = true);
        ~VulkanPipeline();

        void bind() const override;
        void unbind() const override;

        const std::vector<VkDescriptorSet>& get_descriptor_sets() const { return m_descriptor_sets; }
        VkPipeline get_vk_pipeline() const { return m_graphics_pipeline; }
        VkPipelineLayout get_vk_pipeline_layout() const { return m_pipeline_layout; }
        VkRenderPass get_vk_clear_render_pass() const { return m_clear_render_pass; }
        VkRenderPass get_vk_load_render_pass() const { return m_load_render_pass; }
    private:
        std::vector<uint32_t> read_file(const std::string& filepath);
        VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
        void reflect_vertex(const spirv_cross::CompilerGLSL& compiler, VkVertexInputBindingDescription& binding_description, std::vector<VkVertexInputAttributeDescription>& attribute_descriptions);
        void reflect_uniform_buffer(const spirv_cross::CompilerGLSL& compiler, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& ubo_layout_bindings, VkShaderStageFlagBits stage_flag, uint32_t& ubo_count);
        void reflect_sampler(const spirv_cross::CompilerGLSL& compiler, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& sampler_layout_bindings, VkShaderStageFlagBits stage_flag, uint32_t& sampler_count);
        void reflect_output(const spirv_cross::CompilerGLSL& compiler, bool is_last);
    private:
        std::string m_name;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_graphics_pipeline;
        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        std::vector<VkDescriptorSet> m_descriptor_sets;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
        VkRenderPass m_clear_render_pass = VK_NULL_HANDLE;
        VkRenderPass m_load_render_pass = VK_NULL_HANDLE;
    };

}