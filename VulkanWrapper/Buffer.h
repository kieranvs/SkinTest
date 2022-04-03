#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace VulkanWrapper
{
    struct Buffer
    {
        VkBuffer buffer;
        VkDeviceMemory buffer_memory;
        VkDeviceSize buffer_size;
        size_t num_elements;

        void init(VkPhysicalDevice physical_device, VkDevice logical_device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
        void deinit(VkDevice logical_device);
    };

    void copyBuffer(VkDevice logical_device, VkCommandPool command_pool, VkQueue graphics_queue, Buffer& src_buffer, Buffer& dst_buffer);
    void uploadData(Buffer& buffer, VkDevice logical_device, const void* data);

    template<typename T>
    void uploadBufferData(VkPhysicalDevice physical_device, VkDevice logical_device, VkCommandPool command_pool, VkQueue graphics_queue, Buffer& buffer, const std::vector<T>& data, VkBufferUsageFlagBits usage)
    {
        const VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

        Buffer stagingBuffer;
        stagingBuffer.init(physical_device, logical_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uploadData(stagingBuffer, logical_device, data.data());

        buffer.init(physical_device, logical_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer.num_elements = data.size();

        copyBuffer(logical_device, command_pool, graphics_queue, stagingBuffer, buffer);

        stagingBuffer.deinit(logical_device);
    }
}