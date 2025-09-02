#include "VulkanShaderResourceBinding.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

namespace Yogi
{

Scope<IShaderResourceBinding> IShaderResourceBinding::Create(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout)
{
    return CreateScope<VulkanShaderResourceBinding>(shaderResourceLayout);
}

VulkanShaderResourceBinding::VulkanShaderResourceBinding(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout)
{
    View<VulkanDeviceContext> context = static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice                  device  = context->GetVkDevice();

    std::vector<VkDescriptorSetLayoutBinding> bindings(shaderResourceLayout.size());
    for (int i = 0; i < shaderResourceLayout.size(); ++i)
    {
        const auto& attr               = shaderResourceLayout[i];
        bindings[i].binding            = attr.Binding;
        bindings[i].descriptorType     = (attr.Type == ShaderResourceType::Texture) ?
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create pipeline layout");
}

VulkanShaderResourceBinding::~VulkanShaderResourceBinding()
{
    View<VulkanDeviceContext> context = static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    VkDevice                  device  = context->GetVkDevice();
    if (m_descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    if (m_pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void VulkanShaderResourceBinding::BindBuffer(const View<IBuffer>& buffer, int binding, int slot)
{
    View<VulkanBuffer> vkBuffer = static_cast<View<VulkanBuffer>>(buffer);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer->GetVkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range  = vkBuffer->GetSize();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_descriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = slot;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = &bufferInfo;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    View<VulkanDeviceContext> context = static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanShaderResourceBinding::BindTexture(const View<ITexture>& texture, int binding, int slot)
{
    View<VulkanTexture>   vkTexture = static_cast<View<VulkanTexture>>(texture);
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView   = vkTexture->GetVkImageView();
    imageInfo.sampler     = vkTexture->GetVkSampler();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_descriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = slot;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pImageInfo       = &imageInfo;
    descriptorWrite.pBufferInfo      = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    View<VulkanDeviceContext> context = static_cast<View<VulkanDeviceContext>>(Application::GetInstance().GetContext());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

} // namespace Yogi