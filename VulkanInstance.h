#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Vertex.h"

#include <vector>

struct DeviceManager
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkCommandPool command_pool;

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

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory buffer_memory;
    VkDeviceSize buffer_size;

    void init(VkPhysicalDevice physical_device, VkDevice logical_device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
    void deinit(VkDevice logical_device);
};

struct Pipeline
{
    VkRenderPass render_pass;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;
    
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;

    Image colour_image;
    Image depth_image;

    std::vector<Buffer> uniform_buffers;

    std::vector<VkFramebuffer> framebuffers;

    void init(const DeviceManager& device_manager, const Swapchain& swapchain);
    void deinit(const DeviceManager& device_manager);
};

struct SingleTimeCommandBuffer
{
    VkCommandBuffer command_buffer;

    void begin(VkDevice logical_device, VkCommandPool command_pool);
    void end(VkDevice logical_device, VkQueue graphics_queue, VkCommandPool command_pool);
};

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

    void init();
    void deinit();

    void createBuffers(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
};

