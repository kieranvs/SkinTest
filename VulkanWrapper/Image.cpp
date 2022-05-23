#include "Image.h"
#include "Log.h"
#include "Buffer.h"
#include "CommandBuffer.h"

#include <stb/stb_image.h>

namespace VulkanWrapper
{
    static uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        for (uint32_t i{}; i < mem_properties.memoryTypeCount; ++i)
        {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        log_error("failed to find suitable memory type!");
    }

    void Image::createImage(
        VkDevice logical_device,
        VkPhysicalDevice physical_device,
        const uint32_t width,
        const uint32_t height,
        uint32_t mip_map_levels,
        VkSampleCountFlagBits num_samples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
    {
        this->width = width;
        this->height = height;
        this->format = format;
        this->mip_map_levels = mip_map_levels;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_map_levels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = num_samples;
        imageInfo.flags = 0;

        if (vkCreateImage(logical_device, &imageInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("failed to create image!");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(logical_device, handle, &mem_requirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = mem_requirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            log_error("failed to allocate image memory");

        vkBindImageMemory(logical_device, handle, memory, 0);
    }

    void Image::deinit(VkDevice logical_device) {
        vkDestroyImageView(logical_device, view, nullptr);
        vkDestroyImage(logical_device, handle, nullptr);
        vkFreeMemory(logical_device, memory, nullptr);
    }

    void Texture::init(const DeviceManager& device_manager, const std::string& texture_path)
    {
        uploadTextureData(device_manager, image, texture_path);

        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device_manager.physicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(image.mip_map_levels);

            if (vkCreateSampler(device_manager.logicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
                log_error("failed to create texture sampler!");
        }
    }

    void Texture::deinit(VkDevice logical_device)
    {
        image.deinit(logical_device);
        vkDestroySampler(logical_device, sampler, nullptr);
    }

    void createImageView(VkDevice logical_device, Image& image, VkImageAspectFlags aspect_flags)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image.handle;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = image.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspect_flags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = image.mip_map_levels;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logical_device, &createInfo, nullptr, &image.view) != VK_SUCCESS)
            log_error("Failed to create swapchain image views!");
    }

    void transitionLayout(const DeviceManager& device_manager, Image& image, VkImageLayout src_layout, VkImageLayout dest_layout)
    {
        SingleTimeCommandBuffer command_buffer;
        command_buffer.begin(device_manager);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = src_layout;
        barrier.newLayout = dest_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.handle;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = image.mip_map_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if (dest_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (image.format == VK_FORMAT_D32_SFLOAT_S8_UINT || image.format == VK_FORMAT_D24_UNORM_S8_UINT)
                 barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dest_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (src_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dest_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dest_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
            log_error("unsupported layout transtion!");

        vkCmdPipelineBarrier(command_buffer.getHandle(), sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        command_buffer.end(device_manager);
    }

    void uploadTextureData(const DeviceManager& device_manager, Image& image, const std::string& texture_path)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(texture_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        uint32_t mipLevels = 1;

        const VkDeviceSize imageSize = texWidth * texHeight * 4;
        // mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        Buffer stagingBuffer;
        stagingBuffer.init(device_manager.physicalDevice, device_manager.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uploadData(stagingBuffer, device_manager.logicalDevice, pixels);

        stbi_image_free(pixels);

        image.createImage(device_manager.logicalDevice, device_manager.physicalDevice, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        transitionLayout(device_manager, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(device_manager, stagingBuffer, image);

        //generateMipMaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
        transitionLayout(device_manager, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        stagingBuffer.deinit(device_manager.logicalDevice);

        createImageView(device_manager.logicalDevice, image, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}