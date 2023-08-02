#include "backends/renderer/vulkan/vulkan_texture.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include <stb_image.h>

namespace Yogi {

    Ref<RenderTexture> RenderTexture::create(const std::string& name, uint32_t width, uint32_t height, TextureFormat format)
    {
        return CreateRef<VulkanRenderTexture>(name, width, height, format);
    }

    Ref<Texture2D> Texture2D::create(const std::string& name, const std::string& path)
    {
        return CreateRef<VulkanTexture2D>(name, path);
    }

    void init_texture(uint32_t& width, uint32_t& height, VkFormat& internal_format, VkImage& image, VkDeviceMemory& image_memory, VkImageView& image_view, VkSampler& sampler, VkImageUsageFlags usage)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        context->create_image(width, height, internal_format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, image_memory);
        image_view = context->create_image_view(image, internal_format, VK_IMAGE_ASPECT_COLOR_BIT);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        if (internal_format == VK_FORMAT_R8G8B8A8_SRGB) {
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
        }
        else {
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
        }
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkResult result = vkCreateSampler(context->get_device(), &samplerInfo, nullptr, &sampler);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create texture sampler!");
    }

    void read_pixel_impl(int32_t region_width, int32_t region_height, int32_t x, int32_t y, void* data, uint32_t width, uint32_t height, VkFormat internal_format, VkImage image)
    {
        y = region_height - y;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        context->transition_image_layout(image, internal_format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        uint32_t bpp = 0;
        if (internal_format == VK_FORMAT_R8G8B8A8_UNORM) {
            bpp = 4;
        }
        else if (internal_format == VK_FORMAT_R8G8B8_UNORM) {
            bpp = 3;
        }
        else if (internal_format == VK_FORMAT_R32_SINT) {
            bpp = 4;
        }
        else if (internal_format == VK_FORMAT_B8G8R8A8_UNORM) {
            bpp = 4;
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        context->create_buffer(width * height * bpp, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
        void* pdata;
        vkMapMemory(context->get_device(), staging_buffer_memory, 0, width * height * bpp, 0, &pdata);
        
        vkCmdCopyImageToBuffer(
            command_buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            staging_buffer,
            1,
            &region
        );
        context->end_single_time_commands(command_buffer);
        memcpy(data, (uint8_t*)pdata + x * bpp + y * width * bpp, bpp);
        vkUnmapMemory(context->get_device(), staging_buffer_memory);
        context->transition_image_layout(image, internal_format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
        vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
    }

    void set_data_impl(void* data, size_t size, uint32_t& width, uint32_t& height, VkFormat& internal_format, VkImage& image)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        context->transition_image_layout(image, internal_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        context->create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
        void* pdata;
        vkMapMemory(context->get_device(), staging_buffer_memory, 0, size, 0, &pdata);
        memcpy(pdata, data, static_cast<size_t>(size));
        vkUnmapMemory(context->get_device(), staging_buffer_memory);
        vkCmdCopyBufferToImage(
            command_buffer,
            staging_buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        context->end_single_time_commands(command_buffer);
        context->transition_image_layout(image, internal_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
        vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
    }

    void bind_impl(uint32_t binding, uint32_t slot, VkImageView image_view, VkSampler sampler)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        if (context->get_current_pipeline() && !context->get_current_pipeline()->get_descriptor_sets().empty()) {
            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = image_view;
            image_info.sampler = sampler;

            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = context->get_current_pipeline()->get_descriptor_sets()[0];
            descriptor_write.dstBinding = binding;
            descriptor_write.dstArrayElement = slot;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = nullptr;
            descriptor_write.pImageInfo = &image_info;
            descriptor_write.pTexelBufferView = nullptr;

            context->wait_render_command();
            vkUpdateDescriptorSets(context->get_device(), 1, &descriptor_write, 0, nullptr);
        }
    }

    //---------------------------------------------------------------------------------------------------------

    VulkanTexture2D::VulkanTexture2D(const std::string& name, const std::string& path)
    {
        m_name = name;
        stbi_set_flip_vertically_on_load(1);

        int width, height, channels;
        stbi_uc* data = nullptr;
        {
            YG_PROFILE_SCOPE("stbi_load - VulkanTexture2D::VulkanTexture2D(const std::string&)");
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        }
        YG_CORE_ASSERT(data, "Failed to load image!");
        m_width = width;
        m_height = height;
        VkDeviceSize image_size;

        if (channels == 4) {
            m_internal_format = VK_FORMAT_R8G8B8A8_UNORM;
            image_size = m_width * m_height * 4;
        }
        else if (channels == 3) {
            m_internal_format = VK_FORMAT_R8G8B8_UNORM;
            image_size = m_width * m_height * 3;
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        init_texture(m_width, m_height, m_internal_format, m_image, m_image_memory, m_image_view, m_sampler, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        set_data(data, image_size);
        m_digest = MD5(std::string{data, data + image_size}).toStr();

        stbi_image_free(data);
    }

    VulkanTexture2D::~VulkanTexture2D()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        vkDestroySampler(context->get_device(), m_sampler, nullptr);
        vkDestroyImageView(context->get_device(), m_image_view, nullptr);
        vkDestroyImage(context->get_device(), m_image, nullptr);
        vkFreeMemory(context->get_device(), m_image_memory, nullptr);
    }

    void VulkanTexture2D::read_pixel(int32_t width, int32_t height, int32_t x, int32_t y, void* data) const
    {
        read_pixel_impl(width, height, x, y, data, m_width, m_height, m_internal_format, m_image);
    }

    void VulkanTexture2D::set_data(void* data, size_t size)
    {
        set_data_impl(data, size, m_width, m_height, m_internal_format, m_image);
    };

    void VulkanTexture2D::bind(uint32_t binding, uint32_t slot) const
    {
        bind_impl(binding, slot, m_image_view, m_sampler);
    }

    //----------------------------------------------------------------------------------------------------

    VulkanRenderTexture::VulkanRenderTexture(const std::string& name, uint32_t width, uint32_t height, TextureFormat format) : m_width(width), m_height(height)
    {
        m_name = name;
        m_format = format;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        if (format == TextureFormat::RGBA8) {
            m_internal_format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        else if (format == TextureFormat::RED_INTEGER) {
            m_internal_format = VK_FORMAT_R32_SINT;
        }
        else if (format == TextureFormat::ATTACHMENT) {
            m_internal_format = context->get_swap_chain_image_format();
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        init_texture(m_width, m_height, m_internal_format, m_image, m_image_memory, m_image_view, m_sampler, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        context->transition_image_layout(m_image, m_internal_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VulkanRenderTexture::VulkanRenderTexture(uint32_t width, uint32_t height, VkImage vk_image, VkFormat vk_format)
     : m_width(width), m_height(height), m_image(vk_image), m_internal_format(vk_format)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        m_image_view = context->create_image_view(m_image, m_internal_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VulkanRenderTexture::~VulkanRenderTexture()
    {
        auto s = m_name;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        if (m_sampler) {
            vkDestroySampler(context->get_device(), m_sampler, nullptr);
            vkDestroyImageView(context->get_device(), m_image_view, nullptr);
            vkDestroyImage(context->get_device(), m_image, nullptr);
            vkFreeMemory(context->get_device(), m_image_memory, nullptr);
        }
        else {
            vkDestroyImageView(context->get_device(), m_image_view, nullptr);
        }
    }

    void VulkanRenderTexture::read_pixel(int32_t width, int32_t height, int32_t x, int32_t y, void* data) const
    {
        read_pixel_impl(width, height, x, y, data, m_width, m_height, m_internal_format, m_image);
    }

    void VulkanRenderTexture::set_data(void* data, size_t size)
    {
        set_data_impl(data, size, m_width, m_height, m_internal_format, m_image);
    };

    void VulkanRenderTexture::bind(uint32_t binding, uint32_t slot) const
    {
        bind_impl(binding, slot, m_image_view, m_sampler);
    }

    void VulkanRenderTexture::resize(uint32_t width, uint32_t height)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        if (m_sampler) {
            vkDestroySampler(context->get_device(), m_sampler, nullptr);
            vkDestroyImageView(context->get_device(), m_image_view, nullptr);
            vkDestroyImage(context->get_device(), m_image, nullptr);
            vkFreeMemory(context->get_device(), m_image_memory, nullptr);
        }
        else {
            vkDestroyImageView(context->get_device(), m_image_view, nullptr);
        }

        m_width = width;
        m_height = height;
        init_texture(m_width, m_height, m_internal_format, m_image, m_image_memory, m_image_view, m_sampler, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        context->transition_image_layout(m_image, m_internal_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        VkCommandBuffer command_buffer = context->begin_single_time_commands();
        VkClearColorValue clear_color = {{ 0.1f, 0.1f, 0.1f, 1.0f }};
        VkImageSubresourceRange srRange = {};
        srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srRange.baseMipLevel = 0;
        srRange.levelCount = VK_REMAINING_MIP_LEVELS;
        srRange.baseArrayLayer = 0;
        srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        vkCmdClearColorImage(command_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &srRange);
        context->end_single_time_commands(command_buffer);

        context->transition_image_layout(m_image, m_internal_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

}