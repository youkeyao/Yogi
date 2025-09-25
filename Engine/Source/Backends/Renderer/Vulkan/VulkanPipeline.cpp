#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanShaderResourceBinding.h"
#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

Handle<IPipeline> IPipeline::Create(const PipelineDesc& desc) { return Handle<VulkanPipeline>::Create(desc); }

VulkanPipeline::VulkanPipeline(const PipelineDesc& desc)
{
    m_desc = desc;
    CreateVkPipeline(desc);
}

VulkanPipeline::~VulkanPipeline()
{
    if (m_pipeline != VK_NULL_HANDLE)
    {
        VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
        vkDestroyPipeline(context->GetVkDevice(), m_pipeline, nullptr);
    }
}

void VulkanPipeline::CreateVkPipeline(const PipelineDesc& desc)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkDevice             device  = context->GetVkDevice();

    // --- Shader modules ---
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule>                  shaderModules;
    for (const auto& shader : desc.Shaders)
    {
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = shader->Code.size();
        moduleCreateInfo.pCode    = reinterpret_cast<const uint32_t*>(shader->Code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            YG_CORE_ERROR("Vulkan: Failed to create {0} shader module!", (int)shader->Stage);
            continue;
        }
        shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage  = YgShaderStage2VkShaderStage(shader->Stage);
        stageInfo.module = shaderModule;
        stageInfo.pName  = "main";
        shaderStages.push_back(stageInfo);
    }

    // --- Vertex input ---
    std::vector<VkVertexInputAttributeDescription> attributeDescs;
    VkVertexInputBindingDescription                bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = 0;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    for (const auto& attr : desc.VertexLayout)
    {
        VkVertexInputAttributeDescription vkAttr{};
        vkAttr.binding  = 0;
        vkAttr.location = static_cast<uint32_t>(attributeDescs.size());
        vkAttr.offset   = attr.Offset;
        vkAttr.format   = YgShaderElementType2VkFormat(attr.Type);
        attributeDescs.push_back(vkAttr);
        bindingDesc.stride += attr.Size;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = bindingDesc.stride > 0 ? 1 : 0;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescs.data();

    // --- Dynamic States ---
    std::vector<VkDynamicState>      dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR,
                                                       VK_DYNAMIC_STATE_CULL_MODE_EXT };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    // --- Input assembly ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // TODO: map from desc.Topology
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // --- Viewport & scissor (dynamic for now) ---
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    // --- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // --- Multisampling ---
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = (VkSampleCountFlagBits)(desc.RenderPass->GetDesc().NumSamples);

    // --- Color blend ---
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_MAX;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // --- Depth Stencil ---
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.0f;
    depthStencil.maxDepthBounds        = 1.0f;
    depthStencil.stencilTestEnable     = VK_FALSE;

    // --- Graphics pipeline ---
    Ref<VulkanShaderResourceBinding> vkSRB = Ref<VulkanShaderResourceBinding>::Cast(desc.ShaderResourceBinding);
    VkGraphicsPipelineCreateInfo     pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = vkSRB->GetVkPipelineLayout();
    pipelineInfo.renderPass          = Ref<VulkanRenderPass>::Cast(desc.RenderPass)->GetVkRenderPass();
    pipelineInfo.subpass             = 0;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create graphics pipeline");

    // Cleanup shader modules
    for (auto& module : shaderModules)
    {
        vkDestroyShaderModule(device, module, nullptr);
    }
}

} // namespace Yogi
