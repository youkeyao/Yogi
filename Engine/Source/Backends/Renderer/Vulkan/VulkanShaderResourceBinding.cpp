#include "VulkanShaderResourceBinding.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<IShaderResourceBinding> IShaderResourceBinding::Create(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
    const std::vector<PushConstantRange>&       pushConstantRanges)
{
    return Owner<VulkanShaderResourceBinding>::Create(shaderResourceLayout, pushConstantRanges);
}

VulkanShaderResourceBinding::VulkanShaderResourceBinding(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
    const std::vector<PushConstantRange>&       pushConstantRanges)
{
    m_layout             = shaderResourceLayout;
    m_pushConstantRanges = pushConstantRanges;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkDevice             device  = context->GetVkDevice();

    std::vector<VkDescriptorSetLayoutBinding> bindings(shaderResourceLayout.size());
    for (int i = 0; i < shaderResourceLayout.size(); ++i)
    {
        const auto& attr    = shaderResourceLayout[i];
        bindings[i].binding = attr.Binding;
        switch (attr.Type)
        {
            case ShaderResourceType::StorageBuffer:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case ShaderResourceType::Texture:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case ShaderResourceType::StorageImage:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
            default:
                YG_CORE_ERROR("Vulkan: Unsupported shader resource type!");
                break;
        }
        bindings[i].descriptorCount    = attr.Count;
        bindings[i].stageFlags         = YgShaderStage2VkShaderStage(attr.Stage);
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create descriptor set layout!");

    m_descriptorSet = context->AllocateVkDescriptorSet(m_descriptorSetLayout);

    std::vector<VkPushConstantRange> vkPushConstantRanges;
    vkPushConstantRanges.reserve(m_pushConstantRanges.size());
    for (const auto& range : m_pushConstantRanges)
    {
        VkPushConstantRange vkRange{};
        vkRange.stageFlags = YgShaderStage2VkShaderStage(range.Stage);
        vkRange.offset     = range.Offset;
        vkRange.size       = range.Size;
        vkPushConstantRanges.push_back(vkRange);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges    = vkPushConstantRanges.empty() ? nullptr : vkPushConstantRanges.data();

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create pipeline layout");
}

VulkanShaderResourceBinding::~VulkanShaderResourceBinding()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    VkDevice             device  = context->GetVkDevice();
    if (m_descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    if (m_pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void VulkanShaderResourceBinding::BindBuffer(View<IBuffer> buffer, int binding, int slot)
{
    View<VulkanBuffer> vkBuffer = View<VulkanBuffer>::Cast(buffer);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->GetVkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range  = vkBuffer->GetSize();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = m_descriptorSet;
    descriptorWrite.dstBinding      = binding;
    descriptorWrite.dstArrayElement = slot;
    if (vkBuffer->GetUsage() & BufferUsage::Storage)
    {
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (vkBuffer->GetUsage() & BufferUsage::Uniform)
    {
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    else
    {
        YG_CORE_ERROR("Vulkan: Unsupported buffer usage type!");
        return;
    }
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = &bufferInfo;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanShaderResourceBinding::BindTexture(View<ITexture> texture, int binding, int slot, uint32_t mipLevel)
{
    const ShaderResourceAttribute* bindingAttr = nullptr;
    for (const auto& attr : m_layout)
    {
        if (attr.Binding == binding)
        {
            bindingAttr = &attr;
            break;
        }
    }

    if (!bindingAttr)
    {
        YG_CORE_ERROR("Vulkan: Binding {0} not found in shader resource layout", binding);
        return;
    }

    View<VulkanTexture>   vkTexture = View<VulkanTexture>::Cast(texture);
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = vkTexture->GetVkImageView(mipLevel);

    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    switch (bindingAttr->Type)
    {
        case ShaderResourceType::Texture:
            descriptorType        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageInfo.sampler     = vkTexture->GetVkSampler();
            imageInfo.imageLayout = texture->GetUsage() == ITexture::Usage::DepthStencil ?
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case ShaderResourceType::StorageImage:
            descriptorType        = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageInfo.sampler     = VK_NULL_HANDLE;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            break;
        default:
            YG_CORE_ERROR("Vulkan: Binding {0} is not an image resource", binding);
            return;
    }

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_descriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = slot;
    descriptorWrite.descriptorType   = descriptorType;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pImageInfo       = &imageInfo;
    descriptorWrite.pBufferInfo      = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext().Get());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

} // namespace Yogi