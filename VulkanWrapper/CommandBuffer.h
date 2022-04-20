#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"

#include <optional>

namespace VulkanWrapper
{
    struct CommandBufferSet
    {
        std::vector<VkCommandBuffer> handles;
        std::optional<size_t> active_buffer;

        VkCommandBuffer& operator[](const size_t i) { return handles[i]; }

        size_t size() { return handles.size(); }

        void init(DeviceManager& device_manager, size_t num_buffers);
        void deinit(DeviceManager& device_manager);

        void begin(size_t buffer_index, VkCommandBufferUsageFlags flags);
        void end();
    };

    struct SingleTimeCommandBuffer
    {
        CommandBufferSet command_buffer_set;

        VkCommandBuffer& getHandle() { return command_buffer_set[0]; }

        void begin(DeviceManager& device_manager);
        void end(DeviceManager& device_manager);
    };
}