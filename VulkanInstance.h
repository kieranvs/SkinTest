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
};

struct Swapchain
{
    VkExtent2D extent;
    VkSwapchainKHR handle;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat imageFormat;
};

struct VulkanInstance
{
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debugMessenger;

    GLFWwindow* window = nullptr;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surface_capabilities; // maybe should be somewhere else
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> surface_presentModes;

    bool framebufferResized = false;

    DeviceManager device_manager;
    Swapchain swapchain;

    void init();
    void deinit();
};

