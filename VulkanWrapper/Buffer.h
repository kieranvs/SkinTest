#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"

#include <vector>

namespace VulkanWrapper
{
    struct Image;

    struct Buffer
    {
        VkBuffer handle;
        VkDeviceMemory memory;
        VkDeviceSize size_bytes;
        size_t count;

        void init(VkPhysicalDevice physical_device, VkDevice logical_device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
        void deinit(VkDevice logical_device);
    };

    void copyBuffer(DeviceManager& device_manager, Buffer& src_buffer, Buffer& dst_buffer);
    void uploadData(Buffer& buffer, VkDevice logical_device, const void* data);

    void copyBufferToImage(const DeviceManager& device_manager, Buffer& buffer, Image& image);

    template<typename T>
    void uploadBufferData(DeviceManager& device_manager, Buffer& buffer, const std::vector<T>& data, VkBufferUsageFlagBits usage)
    {
        const VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

        Buffer stagingBuffer;
        stagingBuffer.init(device_manager.physicalDevice, device_manager.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uploadData(stagingBuffer, device_manager.logicalDevice, data.data());

        buffer.init(device_manager.physicalDevice, device_manager.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer.count = data.size();

        copyBuffer(device_manager, stagingBuffer, buffer);

        stagingBuffer.deinit(device_manager.logicalDevice);
    }
}