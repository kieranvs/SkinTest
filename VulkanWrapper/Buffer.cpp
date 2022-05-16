#include "Buffer.h"
#include "Image.h"
#include "Log.h"
#include "CommandBuffer.h"

#include <cstring>

namespace VulkanWrapper
{
    static uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
        for (uint32_t i{}; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        log_error("failed to find suitable memory type!");
    }

    void Buffer::init(VkPhysicalDevice physical_device, VkDevice logical_device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
    {
        size_bytes = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logical_device, &bufferInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("failed to create buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logical_device, handle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
            log_error("failed to allocate buffer memory!");

        vkBindBufferMemory(logical_device, handle, memory, 0);
    }

    void Buffer::deinit(VkDevice logical_device)
    {
        vkDestroyBuffer(logical_device, handle, nullptr);
        vkFreeMemory(logical_device, memory, nullptr);
    }

    void copyBuffer(DeviceManager& device_manager, Buffer& src_buffer, Buffer& dst_buffer)
    {
        if (src_buffer.size_bytes != dst_buffer.size_bytes)
            log_error("Mismatched buffer size!");

        SingleTimeCommandBuffer command_buffer;
        command_buffer.begin(device_manager);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = src_buffer.size_bytes;
        vkCmdCopyBuffer(command_buffer.getHandle(), src_buffer.handle, dst_buffer.handle, 1, &copyRegion);

        command_buffer.end(device_manager);
    }

    void copyBufferToImage(const DeviceManager& device_manager, Buffer& buffer, Image& image)
    {
        SingleTimeCommandBuffer command_buffer;
        command_buffer.begin(device_manager);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { image.width, image.height, 1 };

        vkCmdCopyBufferToImage(command_buffer.getHandle(), buffer.handle, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        command_buffer.end(device_manager);
    }

    void uploadData(Buffer& buffer, VkDevice logical_device, const void* data)
    {
        void* ptr;
        vkMapMemory(logical_device, buffer.memory, 0, buffer.size_bytes, 0, &ptr);
        memcpy(ptr, data, (size_t)buffer.size_bytes);
        vkUnmapMemory(logical_device, buffer.memory);
    }
}