#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "VulkanWrapper/Image.h"
#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/CommandBuffer.h"
#include "VulkanWrapper/Swapchain.h"
#include "VulkanWrapper/DeviceManager.h"
#include "VulkanWrapper/Pipeline.h"

#include <vector>
#include <functional>

using namespace VulkanWrapper;

struct VulkanInstance
{
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debugMessenger;

    GLFWwindow* window = nullptr;
    VkSurfaceKHR surface;

    bool framebufferResized = false;

    DeviceManager device_manager;
    Swapchain swapchain;
    Pipeline pipeline;
    DescriptorPool descriptor_pool;

    CommandBufferSet command_buffer_set;

    std::vector<VkSemaphore> image_available_semaphores; // Per frame in flight: swap chain image is available to start being used
    std::vector<VkSemaphore> render_finished_semaphores; // Per frame in flight: signalled when command buffers have finished execution
    std::vector<VkFence> frame_finished_fences; // Per frame in flight
    std::vector<VkFence> image_to_frame_fences; // Per swapchain image

    std::function<void(const VulkanWrapper::Pipeline& pipeline, const size_t i, const VkCommandBuffer command_buffer )> command_buffer_callback;
    std::function<void(size_t image_index, VkDevice logical_device)> update_uniforms_callback;

    std::function<void()> swapchain_recreate_callback;
    std::function<VkCommandBuffer(size_t)> render_frame_callback;

    void init();
    void deinit();

    void createCommandBuffers();

    void mainLoop();

private:
    void recreateSwapChain();
};

