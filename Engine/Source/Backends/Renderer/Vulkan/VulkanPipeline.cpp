#include "VulkanPipeline.h"
#include "VulkanShaderResourceBinding.h"
#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<IPipeline> IPipeline::Create(const PipelineDesc& desc)
{
    return Owner<VulkanPipeline>::Create(desc);
}

VulkanPipeline::VulkanPipeline(const PipelineDesc& desc)
{
    m_type = desc.Type;
    CreateVkPipelineLayout(desc);
    CreateVkPipeline(desc);
}

VulkanPipeline::~VulkanPipeline()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, m_pipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    }
}

void VulkanPipeline::CreateVkPipelineLayout(const PipelineDesc& desc)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    if (desc.ResourceBinding)
    {
        const VulkanShaderResourceBinding* vkSRB =
            static_cast<const VulkanShaderResourceBinding*>(desc.ResourceBinding);
        setLayout = vkSRB->GetVkDescriptorSetLayout();
    }

    std::vector<VkPushConstantRange> vkPushConstantRanges;
    vkPushConstantRanges.reserve(desc.PushConstantRanges.size());
    for (const auto& range : desc.PushConstantRanges)
    {
        VkPushConstantRange vkRange{};
        vkRange.stageFlags = YgShaderStage2VkShaderStage(range.Stage);
        vkRange.offset     = range.Offset;
        vkRange.size       = range.Size;
        vkPushConstantRanges.push_back(vkRange);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = setLayout != VK_NULL_HANDLE ? 1u : 0u;
    pipelineLayoutInfo.pSetLayouts            = setLayout != VK_NULL_HANDLE ? &setLayout : nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges    = vkPushConstantRanges.empty() ? nullptr : vkPushConstantRanges.data();

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create pipeline layout");
}

void VulkanPipeline::CreateVkPipeline(const PipelineDesc& desc)
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    bool isMeshShading = false;

    // --- Shader modules ---
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule>                  shaderModules;
    for (const auto& shader : desc.Shaders)
    {
        if (!shader)
            continue;
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
        stageInfo.stage  = static_cast<VkShaderStageFlagBits>(YgShaderStage2VkShaderStage(shader->Stage));
        stageInfo.module = shaderModule;
        stageInfo.pName  = "main";
        shaderStages.push_back(stageInfo);

        if (shader && (shader->Stage == ShaderStage::Task || shader->Stage == ShaderStage::Mesh))
        {
            isMeshShading = true;
        }
    }

    if (desc.Type == PipelineType::Compute)
    {
        YG_CORE_ASSERT(shaderStages.size() == 1, "Vulkan: Compute pipeline requires exactly one compute shader stage");
        YG_CORE_ASSERT(shaderStages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT,
                       "Vulkan: Compute pipeline shader stage must be compute");

        VkComputePipelineCreateInfo computeInfo{};
        computeInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computeInfo.stage  = shaderStages[0];
        computeInfo.layout = m_pipelineLayout;

        VkResult computeResult =
            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &m_pipeline);
        YG_CORE_ASSERT(computeResult == VK_SUCCESS, "Vulkan: Failed to create compute pipeline");

        for (auto& module : shaderModules)
        {
            vkDestroyShaderModule(device, module, nullptr);
        }
        return;
    }

    // --- Vertex input (not used for mesh shading pipelines) ---
    std::vector<VkVertexInputAttributeDescription> attributeDescs;
    VkVertexInputBindingDescription                bindingDesc{};
    VkPipelineVertexInputStateCreateInfo           vertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo         inputAssembly{};
    if (!isMeshShading)
    {
        bindingDesc.binding   = 0;
        bindingDesc.stride    = 0;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        for (const auto& attr : desc.VertexLayout)
        {
            VkVertexInputAttributeDescription vkAttr{};
            vkAttr.binding  = 0;
            vkAttr.location = static_cast<uint32_t>(attributeDescs.size());
            vkAttr.offset   = attr.Offset;
            vkAttr.format   = YgFormat2VkFormat(attr.Format);
            attributeDescs.push_back(vkAttr);
            bindingDesc.stride += attr.Size;
        }

        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = bindingDesc.stride > 0 ? 1 : 0;
        vertexInputInfo.pVertexBindingDescriptions      = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attributeDescs.data();

        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = YgPrimitiveTopology2VkPrimitiveTopology(desc.Topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    // --- Dynamic States ---
    std::vector<VkDynamicState>      dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

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
    switch (desc.Cull)
    {
        case CullMode::None:
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
        case CullMode::Back:
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case CullMode::Front:
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
    }
    rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // --- Multisampling ---
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = (VkSampleCountFlagBits)desc.Samples;

    // --- Color blend ---
    const uint32_t colorAttachmentCount = static_cast<uint32_t>(desc.ColorTargets.size());

    auto ToVkBlendAttachment = [](const ColorTargetDesc& t) {
        VkPipelineColorBlendAttachmentState att{};
        att.blendEnable         = t.EnableBlend ? VK_TRUE : VK_FALSE;
        att.srcColorBlendFactor = YgBlendFactor2Vk(t.ColorBlend.SrcFactor);
        att.dstColorBlendFactor = YgBlendFactor2Vk(t.ColorBlend.DstFactor);
        att.colorBlendOp        = YgBlendOp2Vk(t.ColorBlend.Op);
        att.srcAlphaBlendFactor = YgBlendFactor2Vk(t.AlphaBlend.SrcFactor);
        att.dstAlphaBlendFactor = YgBlendFactor2Vk(t.AlphaBlend.DstFactor);
        att.alphaBlendOp        = YgBlendOp2Vk(t.AlphaBlend.Op);
        att.colorWriteMask      = YgColorWriteMask2Vk(t.WriteMask);
        return att;
    };

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(colorAttachmentCount);
    for (uint32_t i = 0; i < colorAttachmentCount; ++i)
        blendAttachments[i] = ToVkBlendAttachment(desc.ColorTargets[i]);

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = colorAttachmentCount;
    colorBlending.pAttachments    = blendAttachments.empty() ? nullptr : blendAttachments.data();

    // --- Depth Stencil ---
    auto CompareOp2VkCompareOp = [](CompareOp op) -> VkCompareOp {
        switch (op)
        {
            case CompareOp::Never:
                return VK_COMPARE_OP_NEVER;
            case CompareOp::Less:
                return VK_COMPARE_OP_LESS;
            case CompareOp::Equal:
                return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessOrEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater:
                return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterOrEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                return VK_COMPARE_OP_LESS;
        }
    };

    auto StencilOp2VkStencilOp = [](StencilOp op) -> VkStencilOp {
        switch (op)
        {
            case StencilOp::Keep:
                return VK_STENCIL_OP_KEEP;
            case StencilOp::Zero:
                return VK_STENCIL_OP_ZERO;
            case StencilOp::Replace:
                return VK_STENCIL_OP_REPLACE;
            case StencilOp::IncrementClamp:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case StencilOp::DecrementClamp:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case StencilOp::Invert:
                return VK_STENCIL_OP_INVERT;
            case StencilOp::IncrementWrap:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case StencilOp::DecrementWrap:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            default:
                return VK_STENCIL_OP_KEEP;
        }
    };

    VkStencilOpState vkFront{};
    vkFront.failOp      = StencilOp2VkStencilOp(desc.Stencil.Front.FailOp);
    vkFront.depthFailOp = StencilOp2VkStencilOp(desc.Stencil.Front.DepthFailOp);
    vkFront.passOp      = StencilOp2VkStencilOp(desc.Stencil.Front.PassOp);
    vkFront.compareOp   = CompareOp2VkCompareOp(desc.Stencil.Front.CompareFn);
    vkFront.compareMask = desc.Stencil.ReadMask;
    vkFront.writeMask   = desc.Stencil.WriteMask;
    vkFront.reference   = desc.Stencil.Reference;

    VkStencilOpState vkBack = vkFront;
    if (desc.Stencil.Front.CompareFn != desc.Stencil.Back.CompareFn ||
        desc.Stencil.Front.FailOp != desc.Stencil.Back.FailOp ||
        desc.Stencil.Front.DepthFailOp != desc.Stencil.Back.DepthFailOp ||
        desc.Stencil.Front.PassOp != desc.Stencil.Back.PassOp)
    {
        vkBack.failOp      = StencilOp2VkStencilOp(desc.Stencil.Back.FailOp);
        vkBack.depthFailOp = StencilOp2VkStencilOp(desc.Stencil.Back.DepthFailOp);
        vkBack.passOp      = StencilOp2VkStencilOp(desc.Stencil.Back.PassOp);
        vkBack.compareOp   = CompareOp2VkCompareOp(desc.Stencil.Back.CompareFn);
        vkBack.compareMask = desc.Stencil.ReadMask;
        vkBack.writeMask   = desc.Stencil.WriteMask;
        vkBack.reference   = desc.Stencil.Reference;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = desc.DepthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable      = desc.DepthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp        = CompareOp2VkCompareOp(desc.DepthCompareOp);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.0f;
    depthStencil.maxDepthBounds        = 1.0f;
    depthStencil.stencilTestEnable     = desc.Stencil.Enable ? VK_TRUE : VK_FALSE;
    depthStencil.front                 = vkFront;
    depthStencil.back                  = vkBack;

    // --- Dynamic rendering ---
    std::vector<VkFormat> colorFormats;
    colorFormats.reserve(desc.ColorTargets.size());
    for (const auto& target : desc.ColorTargets)
    {
        colorFormats.push_back(YgFormat2VkFormat(target.Format));
    }

    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.viewMask                = 0;
    renderingCreateInfo.colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size());
    renderingCreateInfo.pColorAttachmentFormats = colorFormats.empty() ? nullptr : colorFormats.data();
    renderingCreateInfo.depthAttachmentFormat =
        desc.DepthFormat == Format::NONE ? VK_FORMAT_UNDEFINED : YgFormat2VkFormat(desc.DepthFormat);
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // --- Graphics pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingCreateInfo;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = isMeshShading ? nullptr : &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = isMeshShading ? nullptr : &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = m_pipelineLayout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE;
    pipelineInfo.subpass             = 0;
    pipelineInfo.pDepthStencilState  = desc.DepthFormat == Format::NONE ? nullptr : &depthStencil;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create graphics pipeline");

    // Cleanup shader modules
    for (auto& module : shaderModules)
    {
        vkDestroyShaderModule(device, module, nullptr);
    }
}

} // namespace Yogi
