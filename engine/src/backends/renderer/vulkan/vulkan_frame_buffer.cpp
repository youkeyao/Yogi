#include "backends/renderer/vulkan/vulkan_frame_buffer.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include "backends/renderer/vulkan/vulkan_texture.h"

namespace Yogi {

    Ref<FrameBuffer> FrameBuffer::create(uint32_t width, uint32_t height, const std::vector<Ref<RenderTexture>>& color_attachments, bool has_depth_attachment)
    {
        return CreateRef<VulkanFrameBuffer>(width, height, color_attachments, has_depth_attachment);
    }

    VulkanFrameBuffer::VulkanFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<RenderTexture>>& color_attachments, bool is_msaa, bool has_depth_attachment, VkImageLayout layout)
    : m_width(width), m_height(height), m_color_attachments(color_attachments), m_is_msaa(is_msaa), m_has_depth_attachment(has_depth_attachment)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        std::vector<VkImageView> attachments(color_attachments.size());
        std::vector<VkFormat> attachment_formats(color_attachments.size());
        if (m_is_msaa) {
            m_msaa_images.resize(color_attachments.size());
            m_msaa_image_memories.resize(color_attachments.size());
            m_msaa_image_views.resize(color_attachments.size());
        }
        for (int32_t i = 0; i < color_attachments.size(); i ++) {
            auto& attachment = color_attachments[i];
            if (attachment->get_width() != width || attachment->get_height() != height)
                attachment->resize(width, height);
            attachments[i] = ((VulkanRenderTexture*)attachment.get())->get_vk_image_view();
            attachment_formats[i] = ((VulkanRenderTexture*)attachment.get())->get_vk_format();
            if (m_is_msaa) {
                context->create_image(width, height, context->get_msaa_samples(), attachment_formats[i], VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_msaa_images[i], m_msaa_image_memories[i]);
                m_msaa_image_views[i] = context->create_image_view(m_msaa_images[i], attachment_formats[i], VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        m_clear_render_pass = context->create_render_pass(attachment_formats, VK_IMAGE_LAYOUT_UNDEFINED, layout, is_msaa, has_depth_attachment);
        m_load_render_pass = context->create_render_pass(attachment_formats, layout, layout, is_msaa, has_depth_attachment);
        create_vk_frame_buffer(attachments);
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        vkDeviceWaitIdle(context->get_device());
        cleanup_vk_frame_buffer();
        vkDestroyRenderPass(context->get_device(), m_clear_render_pass, nullptr);
        vkDestroyRenderPass(context->get_device(), m_load_render_pass, nullptr);
        for (int32_t i = 0; i < m_msaa_images.size(); i ++) {
            vkDestroyImageView(context->get_device(), m_msaa_image_views[i], nullptr);
            vkDestroyImage(context->get_device(), m_msaa_images[i], nullptr);
            vkFreeMemory(context->get_device(), m_msaa_image_memories[i], nullptr);
        }
    }

    void VulkanFrameBuffer::cleanup_vk_frame_buffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        if (m_has_depth_attachment) {
            if (m_depth_image_view) vkDestroyImageView(context->get_device(), m_depth_image_view, nullptr);
            m_depth_image_view = VK_NULL_HANDLE;
            if (m_depth_image) vkDestroyImage(context->get_device(), m_depth_image, nullptr);
            m_depth_image = VK_NULL_HANDLE;
            if (m_depth_image_memory) vkFreeMemory(context->get_device(), m_depth_image_memory, nullptr);
            m_depth_image_memory = VK_NULL_HANDLE;
        }
        if (m_frame_buffer) vkDestroyFramebuffer(context->get_device(), m_frame_buffer, nullptr);
        m_frame_buffer = VK_NULL_HANDLE;
    }
    
    void VulkanFrameBuffer::create_vk_frame_buffer(const std::vector<VkImageView>& image_views)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        cleanup_vk_frame_buffer();
        std::vector<VkImageView> attachments(m_msaa_image_views);

        for (int32_t i = 0; i < image_views.size(); i ++) {
            attachments.push_back(image_views[i]);
        }

        if (m_has_depth_attachment) {
            VkFormat depth_format = context->find_depth_format();
            context->create_image(m_width, m_height, context->get_msaa_samples(), depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depth_image, m_depth_image_memory);
            m_depth_image_view = context->create_image_view(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

            context->transition_image_layout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

            attachments.push_back(m_depth_image_view);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_clear_render_pass;
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
        context->set_current_frame_buffer(this);
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

        std::vector<VkImageView> attachments{};
        for (auto& attachment : m_color_attachments) {
            if (!attachment) continue;
            if (attachment->get_width() != width || attachment->get_height() != height)
                attachment->resize(width, height);
            attachments.push_back(((VulkanRenderTexture*)attachment.get())->get_vk_image_view());
        }
        if (m_is_msaa) {
            VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
            vkDeviceWaitIdle(context->get_device());
            for (int32_t i = 0; i < m_msaa_images.size(); i ++) {
                vkDestroyImageView(context->get_device(), m_msaa_image_views[i], nullptr);
                vkDestroyImage(context->get_device(), m_msaa_images[i], nullptr);
                vkFreeMemory(context->get_device(), m_msaa_image_memories[i], nullptr);
            }
            for (int32_t i = 0; i < m_color_attachments.size(); i ++) {
                VkFormat attachment_format = ((VulkanRenderTexture*)m_color_attachments[i].get())->get_vk_format();
                context->create_image(width, height, context->get_msaa_samples(), attachment_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_msaa_images[i], m_msaa_image_memories[i]);
                m_msaa_image_views[i] = context->create_image_view(m_msaa_images[i], attachment_format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        create_vk_frame_buffer(attachments);
    }

}