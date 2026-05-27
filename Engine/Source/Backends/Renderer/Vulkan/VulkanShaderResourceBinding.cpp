#include "VulkanShaderResourceBinding.h"
#include "VulkanBuffer.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include <volk.h>

namespace Yogi
{

Owner<IShaderResourceBinding> IShaderResourceBinding::Create(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
    const std::vector<ImmutableSamplerDesc>&    immutableSamplers)
{
    return Owner<VulkanShaderResourceBinding>::Create(shaderResourceLayout, immutableSamplers);
}

VulkanShaderResourceBinding::VulkanShaderResourceBinding(
    const std::vector<ShaderResourceAttribute>& shaderResourceLayout,
    const std::vector<ImmutableSamplerDesc>&    immutableSamplers)
{
    m_layout            = shaderResourceLayout;
    m_immutableSamplers = immutableSamplers;

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();

    std::unordered_map<int, SamplerReductionMode> reductionByBinding;
    for (const auto& s : immutableSamplers)
        reductionByBinding[s.Binding] = s.Reduction;

    std::vector<std::vector<VkSampler>> immutableSamplerArrays(shaderResourceLayout.size());
    for (size_t i = 0; i < shaderResourceLayout.size(); ++i)
    {
        const auto& attr = shaderResourceLayout[i];
        if (attr.Type == ShaderResourceType::Sampler)
        {
            auto                       it = reductionByBinding.find(attr.Binding);
            const SamplerReductionMode mode =
                (it != reductionByBinding.end()) ? it->second : SamplerReductionMode::None;
            immutableSamplerArrays[i].assign(static_cast<size_t>(attr.Count), context->GetSampler(mode));
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings(shaderResourceLayout.size());
    std::vector<VkDescriptorBindingFlags>     bindingFlags(shaderResourceLayout.size(), 0);
    for (int i = 0; i < shaderResourceLayout.size(); ++i)
    {
        const auto& attr    = shaderResourceLayout[i];
        bindings[i].binding = attr.Binding;
        switch (attr.Type)
        {
            case ShaderResourceType::StorageBuffer:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case ShaderResourceType::StorageTexture:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
            case ShaderResourceType::SampledTexture:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            case ShaderResourceType::Sampler:
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            default:
                YG_CORE_ERROR("Vulkan: Unsupported shader resource type!");
                break;
        }
        bindings[i].descriptorCount    = attr.Count;
        bindings[i].stageFlags         = YgShaderStage2VkShaderStage(attr.Stage);
        bindings[i].pImmutableSamplers = immutableSamplerArrays[i].empty() ? nullptr : immutableSamplerArrays[i].data();

        bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        if (attr.Type != ShaderResourceType::Sampler)
            bindingFlags[i] |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }

    uint32_t variableDescriptorCount = 0;
    if (!shaderResourceLayout.empty())
    {
        const auto& lastAttr = shaderResourceLayout.back();
        if (lastAttr.Count > 1)
        {
            bindingFlags.back() |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
            variableDescriptorCount = static_cast<uint32_t>(lastAttr.Count);
        }
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount  = static_cast<uint32_t>(bindingFlags.size());
    flagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();
    layoutInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.pNext        = &flagsInfo;

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    YG_CORE_ASSERT(result == VK_SUCCESS, "Vulkan: Failed to create descriptor set layout!");

    m_descriptorSet = context->AllocateVkDescriptorSet(m_descriptorSetLayout, variableDescriptorCount);
}

VulkanShaderResourceBinding::~VulkanShaderResourceBinding()
{
    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VkDevice             device  = context->GetVkDevice();
    if (m_descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
}

void VulkanShaderResourceBinding::BindBuffer(const IBuffer* buffer, int binding, int slot)
{
    const VulkanBuffer& vkBuffer = *static_cast<const VulkanBuffer*>(buffer);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer.GetVkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range  = vkBuffer.GetSize();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = m_descriptorSet;
    descriptorWrite.dstBinding      = binding;
    descriptorWrite.dstArrayElement = slot;
    if (vkBuffer.GetUsage() & BufferUsage::Storage)
    {
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (vkBuffer.GetUsage() & BufferUsage::Uniform)
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

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanShaderResourceBinding::BindTextureView(const ITextureView* view, int binding, int slot)
{
    YG_CORE_ASSERT(view, "Vulkan: BindTextureView called with null view");

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

    const ITexture* tex = view->GetTexture();
    YG_CORE_ASSERT(tex, "Vulkan: BindTextureView called with view whose source texture has been destroyed");

    const VulkanTextureView* vkView = static_cast<const VulkanTextureView*>(view);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = vkView->GetVkImageView();

    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    switch (bindingAttr->Type)
    {
        case ShaderResourceType::SampledTexture:
            descriptorType    = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageInfo.sampler = VK_NULL_HANDLE;
            if (tex->GetUsage() == ITexture::Usage::DepthStencil)
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            else if (tex->GetUsage() == ITexture::Usage::Storage)
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            else
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case ShaderResourceType::StorageTexture:
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

    VulkanDeviceContext* context = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    vkUpdateDescriptorSets(context->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

} // namespace Yogi