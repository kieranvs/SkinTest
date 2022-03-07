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

struct Image
{
    VkImage image;
    VkDeviceMemory image_memory;
    VkImageView image_view;

    void createImage(
        VkDevice logical_device,
        VkPhysicalDevice physical_device,
        const uint32_t width,
        const uint32_t height,
        uint32_t mipMapLevels,
        VkSampleCountFlagBits numSamples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage);

    void deinit(VkDevice logical_device);
};

struct Pipeline
{
    VkRenderPass render_pass;

    VkDescriptorSetLayout descriptor_set_layout;
    
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;

    Image colour_image;
    Image depth_image;

    std::vector<VkFramebuffer> framebuffers;

    void init(const DeviceManager& device_manager, const Swapchain& swapchain);
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

