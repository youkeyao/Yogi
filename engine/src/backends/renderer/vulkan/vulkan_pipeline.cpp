#include "backends/renderer/vulkan/vulkan_pipeline.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"

namespace Yogi {

    static ShaderDataType spirv_type_to_shader_data_type(spirv_cross::SPIRType stype)
    {
        switch (stype.basetype) {
            case spirv_cross::SPIRType::Float:
                if (stype.vecsize == 1) return ShaderDataType::Float;
                else if (stype.vecsize == 2) return ShaderDataType::Float2;
                else if (stype.vecsize == 3) return ShaderDataType::Float3;
                else if (stype.vecsize == 4) return ShaderDataType::Float4;
            case spirv_cross::SPIRType::Int:
                if (stype.vecsize == 1) return ShaderDataType::Int;
                else if (stype.vecsize == 2) return ShaderDataType::Int2;
                else if (stype.vecsize == 3) return ShaderDataType::Int3;
                else if (stype.vecsize == 4) return ShaderDataType::Int4;
            case spirv_cross::SPIRType::Boolean:
                if (stype.vecsize == 1) return ShaderDataType::Bool;
        }
        YG_CORE_ASSERT(false, "Unknown spirv type!");
        return ShaderDataType::None;
    }

    static VkFormat spirv_type_to_vk_format(spirv_cross::SPIRType stype)
    {
        switch (stype.basetype) {
            case spirv_cross::SPIRType::Float:
                if (stype.vecsize == 1) return VK_FORMAT_R32_SFLOAT;
                else if (stype.vecsize == 2) return VK_FORMAT_R32G32_SFLOAT;
                else if (stype.vecsize == 3) return VK_FORMAT_R32G32B32_SFLOAT;
                else if (stype.vecsize == 4) return VK_FORMAT_R32G32B32A32_SFLOAT;
            case spirv_cross::SPIRType::Int:
                if (stype.vecsize == 1) return VK_FORMAT_R32_SINT;
                else if (stype.vecsize == 2) return VK_FORMAT_R32G32_SINT;
                else if (stype.vecsize == 3) return VK_FORMAT_R32G32B32_SINT;
                else if (stype.vecsize == 4) return VK_FORMAT_R32G32B32A32_SINT;
            case spirv_cross::SPIRType::Boolean:
                if (stype.vecsize == 1) return VK_FORMAT_R32_UINT;
        }
        YG_CORE_ASSERT(false, "Unknown spirv type!");
        return VK_FORMAT_UNDEFINED;
    }

    static VkFormat spirv_type_to_vk_image_format(spirv_cross::SPIRType stype)
    {
        switch (stype.basetype) {
            case spirv_cross::SPIRType::Float:
                if (stype.vecsize == 1) return VK_FORMAT_R32_SFLOAT;
                else if (stype.vecsize == 2) return VK_FORMAT_R32G32_SFLOAT;
                else if (stype.vecsize == 3) return VK_FORMAT_R32G32B32_SFLOAT;
                else if (stype.vecsize == 4) return VK_FORMAT_B8G8R8A8_UNORM;
            case spirv_cross::SPIRType::Int:
                if (stype.vecsize == 1) return VK_FORMAT_R32_SINT;
                else if (stype.vecsize == 2) return VK_FORMAT_R32G32_SINT;
                else if (stype.vecsize == 3) return VK_FORMAT_R32G32B32_SINT;
                else if (stype.vecsize == 4) return VK_FORMAT_R32G32B32A32_SINT;
            case spirv_cross::SPIRType::Boolean:
                if (stype.vecsize == 1) return VK_FORMAT_R32_UINT;
        }
        YG_CORE_ASSERT(false, "Unknown spirv type!");
        return VK_FORMAT_UNDEFINED;
    }

    Ref<Pipeline> Pipeline::create(const std::string& name, const std::vector<std::string>& types, bool is_last)
    {
        return CreateRef<VulkanPipeline>(name, types, is_last);
    }

    VulkanPipeline::VulkanPipeline(const std::string& name, const std::vector<std::string>& types, bool is_last) : m_name(name)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        std::vector<VkShaderModule> shader_modules;

        VkVertexInputBindingDescription binding_description{};
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> layout_bindings{};
        uint32_t ubo_count = 0, sampler_count = 0;

        for (auto type : types) {
            auto shader_code = read_file(YG_SHADER_DIR + name + "." + type);
            VkShaderModule shader_module = create_shader_module(shader_code);
            spirv_cross::CompilerGLSL compiler(std::move(shader_code));

            VkPipelineShaderStageCreateInfo shader_stage_info{};
            shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage_info.module = shader_module;
            shader_stage_info.pName = "main";

            if (type == "vert") {
                shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
                reflect_vertex(compiler, binding_description, attribute_descriptions);
            }
            else if (type == "frag") {
                shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                reflect_output(compiler, is_last);
            }
            else {
                YG_CORE_ASSERT(false, "Invalid shader stage!");
            }
            reflect_uniform_buffer(compiler, layout_bindings, shader_stage_info.stage, ubo_count);
            reflect_sampler(compiler, layout_bindings, shader_stage_info.stage, sampler_count);

            shader_stages.push_back(shader_stage_info);
            shader_modules.push_back(shader_module);
        }

        if (!layout_bindings.empty()) {
            m_descriptor_set_layouts.resize(layout_bindings.size());
            for (int32_t i = 0; i < layout_bindings.size(); i ++) {
                VkDescriptorSetLayoutCreateInfo layout_info{};
                layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = layout_bindings[i].size();
                layout_info.pBindings = layout_bindings[i].data();

                VkResult result = vkCreateDescriptorSetLayout(context->get_device(), &layout_info, nullptr, &m_descriptor_set_layouts[i]);
                YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout!");
            }

            std::vector<VkDescriptorPoolSize> pool_sizes{};
            if (ubo_count > 0) {
                pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ubo_count });
            }
            if (sampler_count > 0) {
                pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sampler_count });
            }

            VkDescriptorPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());;
            pool_info.pPoolSizes = pool_sizes.data();
            pool_info.maxSets = static_cast<uint32_t>(m_descriptor_set_layouts.size());

            VkResult result = vkCreateDescriptorPool(context->get_device(), &pool_info, nullptr, &m_descriptor_pool);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool!");

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = m_descriptor_pool;
            alloc_info.descriptorSetCount = static_cast<uint32_t>(m_descriptor_set_layouts.size());
            alloc_info.pSetLayouts = m_descriptor_set_layouts.data();

            m_descriptor_sets.resize(m_descriptor_set_layouts.size());
            result = vkAllocateDescriptorSets(context->get_device(), &alloc_info, m_descriptor_sets.data());
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate descriptor sets!");

            #ifdef YG_DEBUG
                for (int32_t i = 0; i < layout_bindings.size(); i ++) {
                    for (int32_t j = 0; j < layout_bindings[i].size(); j ++) {
                        std::vector<VkDescriptorBufferInfo> buffer_infos;
                        std::vector<VkDescriptorImageInfo> image_infos;
                        for (int32_t k = 0; k < layout_bindings[i][j].descriptorCount; k ++) {
                            VkDescriptorBufferInfo buffer_info{};
                            buffer_info.buffer = context->get_tmp_uniform_buffer()->get_vk_buffer();
                            buffer_info.offset = 0;
                            buffer_info.range = 1;
                            buffer_infos.push_back(buffer_info);
                            VkDescriptorImageInfo image_info{};
                            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            image_info.imageView = context->get_tmp_texture()->get_vk_image_view();
                            image_info.sampler = context->get_tmp_texture()->get_vk_sampler();
                            image_infos.push_back(image_info);
                        }
                        VkWriteDescriptorSet descriptor_write{};
                        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptor_write.dstSet = m_descriptor_sets[i];
                        descriptor_write.dstBinding = layout_bindings[i][j].binding;
                        descriptor_write.dstArrayElement = 0;
                        descriptor_write.descriptorType = layout_bindings[i][j].descriptorType;
                        descriptor_write.descriptorCount = layout_bindings[i][j].descriptorCount;
                        descriptor_write.pBufferInfo = buffer_infos.data();
                        descriptor_write.pImageInfo = image_infos.data();
                        descriptor_write.pTexelBufferView = nullptr;

                        vkUpdateDescriptorSets(context->get_device(), 1, &descriptor_write, 0, nullptr);
                    }
                }
            #endif
        }

        std::vector<VkDynamicState> dynamic_states = context->get_dynamic_states();
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = binding_description.stride > 0 ? 1 : 0;
        vertex_input_info.pVertexBindingDescriptions = &binding_description;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_MAX;
        std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
        for (auto& element : m_output_layout.get_elements()) {
            color_blend_attachment.blendEnable = element.type == ShaderDataType::Float4 ? VK_TRUE : VK_FALSE;
            color_blend_attachments.push_back(color_blend_attachment);
        }

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = static_cast<uint32_t>(color_blend_attachments.size());
        color_blending.pAttachments = color_blend_attachments.data();
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = VK_TRUE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0.0f;
        depth_stencil.maxDepthBounds = 1.0f;
        depth_stencil.stencilTestEnable = VK_FALSE;

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = m_descriptor_set_layouts.size();
        pipeline_layout_info.pSetLayouts = m_descriptor_set_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = 0;
        pipeline_layout_info.pPushConstantRanges = nullptr;

        VkResult result = vkCreatePipelineLayout(context->get_device(), &pipeline_layout_info, nullptr, &m_pipeline_layout);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shader_stages.size();
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = m_pipeline_layout;
        pipeline_info.renderPass = m_clear_render_pass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.pDepthStencilState = &depth_stencil;

        result = vkCreateGraphicsPipelines(context->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create graphics pipeline!");

        for (auto shader_module : shader_modules) {
            vkDestroyShaderModule(context->get_device(), shader_module, nullptr);
        }
    }

    VulkanPipeline::~VulkanPipeline()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        if (m_descriptor_pool) vkDestroyDescriptorPool(context->get_device(), m_descriptor_pool, nullptr);
        for (auto descriptor_set_layout : m_descriptor_set_layouts) {
            vkDestroyDescriptorSetLayout(context->get_device(), descriptor_set_layout, nullptr);
        }
        vkDestroyPipeline(context->get_device(), m_graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(context->get_device(), m_pipeline_layout, nullptr);
        vkDestroyRenderPass(context->get_device(), m_clear_render_pass, nullptr);
        vkDestroyRenderPass(context->get_device(), m_load_render_pass, nullptr);
    }

    std::vector<uint32_t> VulkanPipeline::read_file(const std::string& filepath)
    {
        std::vector<uint32_t> buffer;
        std::ifstream in(filepath, std::ios::ate | std::ios::binary);

        if (!in.is_open()) {
            YG_CORE_ERROR("Could not open file '{0}'!", filepath);
            return buffer;
        }

        in.seekg(0, std::ios::end);
        buffer.resize(in.tellg() / sizeof(uint32_t));
        in.seekg(0, std::ios::beg);
        in.read((char*)buffer.data(), buffer.size() * sizeof(uint32_t));
        in.close();

        return buffer;
    }

    VkShaderModule VulkanPipeline::create_shader_module(const std::vector<uint32_t>& code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size() * sizeof(uint32_t);
        create_info.pCode = code.data();

        VkShaderModule shader_module;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        VkResult result = vkCreateShaderModule(context->get_device(), &create_info, nullptr, &shader_module);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create shader module!");
        return shader_module;
    }

    void VulkanPipeline::reflect_vertex(const spirv_cross::CompilerGLSL& compiler, VkVertexInputBindingDescription& binding_description, std::vector<VkVertexInputAttributeDescription>& attribute_descriptions)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        std::map<uint32_t, spirv_cross::Resource*> stage_inputs;
        for (auto& stage_input : resources.stage_inputs) {
            stage_inputs[compiler.get_decoration(stage_input.id, spv::DecorationLocation)] = &stage_input;
        }
        for (auto& [location, p_stage_input] : stage_inputs) {
            auto& stage_input = *p_stage_input;
            const auto& input_type = compiler.get_type(stage_input.base_type_id);

            m_vertex_layout.add_element({ spirv_type_to_shader_data_type(input_type), stage_input.name });

            VkVertexInputAttributeDescription attribute_description;
            attribute_description.binding = compiler.get_decoration(stage_input.id, spv::DecorationBinding);
            attribute_description.location = location;
            attribute_description.format = spirv_type_to_vk_format(input_type);
            attribute_description.offset = m_vertex_layout.get_elements().back().offset;

            attribute_descriptions.push_back(attribute_description);
        }

        binding_description.binding = 0;
        binding_description.stride = m_vertex_layout.get_stride();
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    void VulkanPipeline::reflect_uniform_buffer(const spirv_cross::CompilerGLSL& compiler, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& ubo_layout_bindings, VkShaderStageFlagBits stage_flag, uint32_t& ubo_count)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        for (auto& uniform_buffer : resources.uniform_buffers) {
            auto &uniform_type = compiler.get_type(uniform_buffer.type_id);

            uint32_t set = compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
            while (ubo_layout_bindings.size() < set + 1) ubo_layout_bindings.push_back(std::vector<VkDescriptorSetLayoutBinding>{});

            uint32_t array_count = uniform_type.array_size_literal[0] ? uniform_type.array[0] : 1;

            VkDescriptorSetLayoutBinding ubo_layout_binding{};
            ubo_layout_binding.binding = binding;
            ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo_layout_binding.descriptorCount = array_count;
            ubo_layout_binding.stageFlags = stage_flag;
            ubo_layout_binding.pImmutableSamplers = nullptr;

            ubo_layout_bindings[set].push_back(ubo_layout_binding);

            PipelineLayout uniform_layout;
            for (int32_t i = 0; i < uniform_type.member_types.size(); i ++) {
                const auto& member_type = compiler.get_type(uniform_type.member_types[i]);
                uniform_layout.add_element({ spirv_type_to_shader_data_type(member_type), compiler.get_member_name(uniform_type.self, i) });
            }
            m_uniform_layouts[binding] = uniform_layout;

            ubo_count += array_count;
        }
    }

    void VulkanPipeline::reflect_sampler(const spirv_cross::CompilerGLSL& compiler, std::vector<std::vector<VkDescriptorSetLayoutBinding>>& sampler_layout_bindings, VkShaderStageFlagBits stage_flag, uint32_t& sampler_count)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        for (auto& sampled_image : resources.sampled_images) {
            auto &sampler_type = compiler.get_type(sampled_image.type_id);

            uint32_t set = compiler.get_decoration(sampled_image.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(sampled_image.id, spv::DecorationBinding);
            while (sampler_layout_bindings.size() < set + 1) sampler_layout_bindings.push_back(std::vector<VkDescriptorSetLayoutBinding>{});

            uint32_t array_count = sampler_type.array_size_literal[0] ? sampler_type.array[0] : 1;

            VkDescriptorSetLayoutBinding sampler_layout_binding{};
            sampler_layout_binding.binding = binding;
            sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sampler_layout_binding.descriptorCount = array_count;
            sampler_layout_binding.stageFlags = stage_flag;
            sampler_layout_binding.pImmutableSamplers = nullptr;

            sampler_layout_bindings[set].push_back(sampler_layout_binding);

            sampler_count += array_count;
        }
    }

    void VulkanPipeline::reflect_output(const spirv_cross::CompilerGLSL& compiler, bool is_last)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        std::map<uint32_t, spirv_cross::Resource*> stage_outputs;
        for (auto& stage_output : resources.stage_outputs) {
            stage_outputs[compiler.get_decoration(stage_output.id, spv::DecorationLocation)] = &stage_output;
        }
        std::vector<VkFormat> color_attachment_formats;
        for (auto& [location, p_stage_output] : stage_outputs) {
            auto& stage_output = *p_stage_output;
            const auto& output_type = compiler.get_type(stage_output.base_type_id);

            m_output_layout.add_element({ spirv_type_to_shader_data_type(output_type), stage_output.name });

            color_attachment_formats.push_back(spirv_type_to_vk_image_format(output_type));
        }
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        VkImageLayout final_layout = is_last? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_clear_render_pass = context->create_render_pass(color_attachment_formats, VK_IMAGE_LAYOUT_UNDEFINED, final_layout);
        m_load_render_pass = context->create_render_pass(color_attachment_formats, final_layout, final_layout);
    }

    void VulkanPipeline::bind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_pipeline(this);
    }

    void VulkanPipeline::unbind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_pipeline(nullptr);
    }

}