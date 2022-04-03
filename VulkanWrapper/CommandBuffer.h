#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VulkanWrapper
{
    struct SingleTimeCommandBuffer
    {
        VkCommandBuffer command_buffer;

        void begin(VkDevice logical_device, VkCommandPool command_pool);
        void end(VkDevice logical_device, VkQueue graphics_queue, VkCommandPool command_pool);
    };
}