#include "CommandBuffer.h"

#include "DeviceManager.h"
#include "Log.h"

namespace VulkanWrapper
{
    void CommandBufferSet::init(DeviceManager& device_manager, size_t num_buffers)
    {
        handles.resize(num_buffers);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device_manager.command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)handles.size();

        if (vkAllocateCommandBuffers(device_manager.logicalDevice, &allocInfo, handles.data()) != VK_SUCCESS)
            log_error("failed to allocate command buffers!");
    }

    void CommandBufferSet::deinit(DeviceManager& device_manager)
    {
        vkFreeCommandBuffers(device_manager.logicalDevice, device_manager.command_pool, static_cast<uint32_t>(handles.size()), handles.data());
    }

    void CommandBufferSet::begin(size_t buffer_index, VkCommandBufferUsageFlags flags)
    {
        if (active_buffer.has_value())
            log_error("Mismatched CommandBufferSet::begin()/end()");
        active_buffer = buffer_index;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(handles[buffer_index], &beginInfo) != VK_SUCCESS)
            log_error("failed to begin recording command buffer!");
    }

    void CommandBufferSet::end()
    {
        if (!active_buffer.has_value())
            log_error("Mismatched CommandBufferSet::begin()/end()");

        if (vkEndCommandBuffer(handles[active_buffer.value()]) != VK_SUCCESS)
            log_error("failed to record command buffer!");

        active_buffer.reset();
    }

    void SingleTimeCommandBuffer::begin(DeviceManager& device_manager)
    {
        command_buffer_set.init(device_manager, 1);
        command_buffer_set.begin(0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    void SingleTimeCommandBuffer::end(DeviceManager& device_manager)
    {
        command_buffer_set.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer_set[0];

        vkQueueSubmit(device_manager.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(device_manager.graphicsQueue);

        command_buffer_set.deinit(device_manager);
    }
}