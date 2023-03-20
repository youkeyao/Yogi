#include "backends/renderer/vulkan/vulkan_frame_buffer.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include "backends/renderer/vulkan/vulkan_texture.h"

namespace Yogi {

    Ref<FrameBuffer> FrameBuffer::create(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments)
    {
        return CreateRef<VulkanFrameBuffer>(width, height, color_attachments);
    }

    VulkanFrameBuffer::VulkanFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments) : m_width(width), m_height(height)
    {
        YG_CORE_ASSERT(0 < color_attachments.size() && color_attachments.size() <= 4, "Wrong color attachments size!");

        for (auto& attachment : color_attachments) {
            m_color_attachments[m_color_attachments_size] = attachment;
            m_color_attachments_size ++;
        }
        
        create_frame_buffer();
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        vkDestroyImageView(context->get_device(), m_depth_image_view, nullptr);
        vkDestroyImage(context->get_device(), m_depth_image, nullptr);
        vkFreeMemory(context->get_device(), m_depth_image_memory, nullptr);
        vkDestroyFramebuffer(context->get_device(), m_frame_buffer, nullptr);
    }
    
    void VulkanFrameBuffer::create_frame_buffer()
    {
        std::vector<VkImageView> attachments{};

        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        for (auto& attachment : m_color_attachments) {
            if (!attachment) continue;
            attachments.push_back(((VulkanTexture2D*)attachment.get())->get_vk_image_view());
        }

        VkFormat depth_format = context->find_depth_format();
        context->create_image(m_width, m_height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depth_image, m_depth_image_memory);
        m_depth_image_view = context->create_image_view(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

        context->transition_image_layout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

        attachments.push_back(m_depth_image_view);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = context->get_current_pipeline()->get_vk_clear_render_pass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_width;
        framebufferInfo.height = m_height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(context->get_device(), &framebufferInfo, nullptr, &m_frame_buffer);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer!");
    }

    void VulkanFrameBuffer::bind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_frame_buffer(m_frame_buffer);
    }

    void VulkanFrameBuffer::unbind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_frame_buffer(nullptr);
    }

    void VulkanFrameBuffer::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        vkDestroyFramebuffer(context->get_device(), m_frame_buffer, nullptr);

        create_frame_buffer();
    }

    void VulkanFrameBuffer::add_color_attachment(uint32_t index, const Ref<Texture2D>& attachment)
    {
        YG_CORE_ASSERT(index < 4 && !m_color_attachments[index], "Invalid attachment index!");

        m_color_attachments[index] = attachment;
        m_color_attachments_size ++;

        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        vkDestroyFramebuffer(context->get_device(), m_frame_buffer, nullptr);

        create_frame_buffer();
    }

    void VulkanFrameBuffer::remove_color_attachment(uint32_t index)
    {
        YG_CORE_ASSERT(index < 4 && m_color_attachments[index], "Invalid attachment index!");
        m_color_attachments[index] = nullptr;
        m_color_attachments_size --;
    }

    const Ref<Texture2D>& VulkanFrameBuffer::get_color_attachment(uint32_t index) const
    {
        YG_CORE_ASSERT(index < 4 && m_color_attachments[index], "Invalid attachment index!");
        return m_color_attachments[index];
    };

}