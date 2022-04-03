#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "DeviceManager.h"

#include <optional>

namespace VulkanWrapper
{
    struct CommandBufferSet
    {
        std::vector<VkCommandBuffer> command_buffers;
        std::optional<size_t> active_buffer;

        void init(DeviceManager& device_manager, size_t num_buffers);
        void deinit(DeviceManager& device_manager);

        void begin(size_t buffer_index, VkCommandBufferUsageFlags flags);
        void end();
    };

    struct SingleTimeCommandBuffer
    {
        CommandBufferSet command_buffer;

        void begin(DeviceManager& device_manager);
        void end(DeviceManager& device_manager);
    };
}