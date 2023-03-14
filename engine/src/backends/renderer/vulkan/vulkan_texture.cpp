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

        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        VkResult result = vkCreateImage(context->get_device(), &image_info, nullptr, &m_image);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image!");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(context->get_device(), m_image, &mem_requirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = mem_requirements.size;
        allocInfo.memoryTypeIndex = context->find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(context->get_device(), &allocInfo, nullptr, &m_image_memory);
        YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate image memory!");

        vkBindImageMemory(context->get_device(), m_image, m_image_memory, 0);
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
        VkDeviceSize image_size = m_width * m_height * 4;

        if (channels == 4) {
            m_internal_format = VK_FORMAT_R8G8B8A8_SRGB;
        }
        else if (channels == 3) {
            m_internal_format = VK_FORMAT_R8G8B8_SRGB;
        }
        else {
            YG_CORE_ERROR("Invalid texture format!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* pdata;
        vkMapMemory(context->get_device(), stagingBufferMemory, 0, image_size, 0, &pdata);
        memcpy(pdata, data, static_cast<size_t>(image_size));
        vkUnmapMemory(context->get_device(), stagingBufferMemory);

        stbi_image_free(pdata);
        vkDestroyBuffer(context->get_device(), stagingBuffer, nullptr);
        vkFreeMemory(context->get_device(), stagingBufferMemory, nullptr);
    }

    VulkanTexture2D::~VulkanTexture2D()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        vkDestroyImage(context->get_device(), m_image, nullptr);
        vkFreeMemory(context->get_device(), m_image_memory, nullptr);
    }

    void VulkanTexture2D::read_pixel(int32_t x, int32_t y, void* data) const
    {
    }

    void VulkanTexture2D::set_data(void* data, size_t size)
    {
    };

    void VulkanTexture2D::bind(uint32_t slot) const
    {
    }

}