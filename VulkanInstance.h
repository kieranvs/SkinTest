#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct DeviceManager
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkDevice logicalDevice;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> surface_presentModes;
    VkFormat depth_format;

    VkSampleCountFlagBits msaaSamples;

    void init(VkInstance& instance, VkSurfaceKHR& surface);
    void deinit();
};

struct Swapchain
{
    VkExtent2D extent;
    VkSwapchainKHR handle;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat imageFormat;

    void init(const DeviceManager& device_manager, GLFWwindow* window, VkSurfaceKHR surface);
    void deinit(const DeviceManager& device_manager);
};

struct Pipeline
{
    VkRenderPass render_pass;

    void init(const DeviceManager& device_manager, VkFormat swapchain_format, VkFormat depth_format);
    void deinit(const DeviceManager& device_manager);
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

    void init();
    void deinit();
};

