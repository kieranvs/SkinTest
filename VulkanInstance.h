#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Vertex.h"
#include "VulkanWrapper/Image.h"
#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/CommandBuffer.h"
#include "VulkanWrapper/Swapchain.h"
#include "VulkanWrapper/DeviceManager.h"
#include "VulkanWrapper/Pipeline.h"

#include <vector>

using namespace VulkanWrapper;

struct UniformData
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

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

    Buffer vertex_buffer;
    Buffer index_buffer;

    CommandBufferSet command_buffer_set;

    std::vector<VkSemaphore> image_available_semaphores; // Per frame in flight: swap chain image is available to start being used
    std::vector<VkSemaphore> render_finished_semaphores; // Per frame in flight: signalled when command buffers have finished execution
    std::vector<VkFence> frame_finished_fences; // Per frame in flight
    std::vector<VkFence> image_to_frame_fences; // Per swapchain image

    void init();
    void deinit();

    void createBuffers(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void createCommandBuffers();

    void mainLoop();

private:
    void recreateSwapChain();
};

