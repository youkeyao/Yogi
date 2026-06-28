
#pragma once

#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ITexture.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanShaderResourceBinding : public IShaderResourceBinding
{
public:
    VulkanShaderResourceBinding(const std::vector<ShaderResourceAttribute>&     shaderResourceLayout,
                                const std::vector<ImmutableSamplerBindingDesc>& immutableSamplers);
    virtual ~VulkanShaderResourceBinding();

    void BindBuffer(const IBuffer* buffer, int binding, int slot = 0) override;
    void BindTextureView(const ITextureView* view, int binding, int slot = 0) override;
    void BindSampler(const ISampler* sampler, int binding, int slot = 0) override;

    inline VkDescriptorSet       GetVkDescriptorSet() const { return m_descriptorSet; }
    inline VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    VkDescriptorSet        m_descriptorSet       = VK_NULL_HANDLE;
    VkDescriptorSetLayout  m_descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkSampler> m_ownedSamplers; // immutable samplers baked into the layout, owned here
};

} // namespace Yogi
