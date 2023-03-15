#include "backends/renderer/vulkan/vulkan_texture.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include <stb_image.h>

namespace Yogi {

    Ref<Texture2D> Texture2D::create(uint32_t width, uint32_t height, TextureFormat format)
    {
        return CreateRef<VulkanTexture2D>(width, height, format);
    }

    Ref<Texture2D> Texture2D::create(const std::string& path)
    {
        return CreateRef<VulkanTexture2D>(path);
    }

    VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, TextureFormat format) : m_width(width), m_height(height)
    {
        if (format == TextureFormat::RGBA8) {
            m_internal_format = VK_FORMAT_R8G8B8A8_SRGB;
        }
        else if (format == TextureFormat::RED_INTEGER) {
            m_internal_format = VK_FORMAT_R32_SINT;
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        create_image();
    }

    VulkanTexture2D::VulkanTexture2D(const std::string& path)
    {
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
            m_internal_format = VK_FORMAT_R8G8B8A8_SRGB;
            image_size = m_width * m_height * 4;
        }
        else if (channels == 3) {
            m_internal_format = VK_FORMAT_R8G8B8_SRGB;
            image_size = m_width * m_height * 3;
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        create_image();
        set_data(data, image_size);

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

    void VulkanTexture2D::create_image()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = m_width;
        image_info.extent.height = m_height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = m_internal_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0;

        VkResult result = vkCreateImage(context->get_device(), &image_info, nullptr, &m_image);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(context->get_device(), m_image, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = context->find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(context->get_device(), &alloc_info, nullptr, &m_image_memory);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate image memory!");

        vkBindImageMemory(context->get_device(), m_image, m_image_memory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_internal_format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(context->get_device(), &viewInfo, nullptr, &m_image_view);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create texture image view!");

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        if (m_internal_format == VK_FORMAT_R8G8B8A8_SRGB) {
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

        result = vkCreateSampler(context->get_device(), &samplerInfo, nullptr, &m_sampler);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create texture sampler!");
    }

    void VulkanTexture2D::read_pixel(int32_t x, int32_t y, void* data) const
    {
    }

    void VulkanTexture2D::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        VkCommandBuffer commandBuffer = context->begin_single_time_commands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            YG_CORE_ASSERT(false, "Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        context->end_single_time_commands(commandBuffer);
    }

    void VulkanTexture2D::set_data(void* data, size_t size)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        transition_image_layout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
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
            m_width,
            m_height,
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
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        context->end_single_time_commands(command_buffer);
        transition_image_layout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
        vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
    };

    void VulkanTexture2D::bind(uint32_t slot) const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        if (context->get_current_pipeline() && !context->get_current_pipeline()->get_descriptor_sets().empty()) {
            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = m_image_view;
            image_info.sampler = m_sampler;

            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = context->get_current_pipeline()->get_descriptor_sets()[0];
            descriptor_write.dstBinding = slot;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = nullptr;
            descriptor_write.pImageInfo = &image_info;
            descriptor_write.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(context->get_device(), 1, &descriptor_write, 0, nullptr);
        }
    }

}