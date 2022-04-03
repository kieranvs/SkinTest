#include "Buffer.h"
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
        buffer_size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            log_error("failed to create buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logical_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS)
            log_error("failed to allocate buffer memory!");

        vkBindBufferMemory(logical_device, buffer, buffer_memory, 0);
    }

    void Buffer::deinit(VkDevice logical_device)
    {
        vkDestroyBuffer(logical_device, buffer, nullptr);
        vkFreeMemory(logical_device, buffer_memory, nullptr);
    }

    void copyBuffer(DeviceManager& device_manager, Buffer& src_buffer, Buffer& dst_buffer)
    {
        if (src_buffer.buffer_size != dst_buffer.buffer_size)
            log_error("Mismatched buffer size!");

        SingleTimeCommandBuffer command_buffer;
        command_buffer.begin(device_manager);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = src_buffer.buffer_size;
        vkCmdCopyBuffer(command_buffer.command_buffer.command_buffers[0], src_buffer.buffer, dst_buffer.buffer, 1, &copyRegion);

        command_buffer.end(device_manager);
    }

    void uploadData(Buffer& buffer, VkDevice logical_device, const void* data)
    {
        void* ptr;
        vkMapMemory(logical_device, buffer.buffer_memory, 0, buffer.buffer_size, 0, &ptr);
        memcpy(ptr, data, (size_t)buffer.buffer_size);
        vkUnmapMemory(logical_device, buffer.buffer_memory);
    }
}