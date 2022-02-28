#include "Vertex.h"
#include "VulkanInstance.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <algorithm>
#include <array>
#include <fstream>

void log_warning(const char* message)
{
    printf(message);
}

void log_error(const char* message)
{
    printf(message);
    exit(-1);
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto instance = static_cast<VulkanInstance*>(glfwGetWindowUserPointer(window));
    instance->framebufferResized = true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

const uint32_t window_width = 800;
const uint32_t window_height = 600;
const bool enable_validation_layers = true;
const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

std::vector<const char*> getRequiredExtensions()
{
    std::vector<const char*> extensions;

    // GLFW required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        extensions.push_back(glfwExtensions[i]);

    if (enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool checkValidationLayerSupport()
{
    if (!enable_validation_layers) return true;

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> availableLayers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, availableLayers.data());

    auto check_required_layer = [&availableLayers](const char* required_layer_name)
    {
        for (const auto& available_layer_properties : availableLayers)
        {
            if (strcmp(required_layer_name, available_layer_properties.layerName) == 0)
                return true;
        }
        return false;
    };

    for (const char* required_layer_name : validation_layers)
    {
        if (!check_required_layer(required_layer_name))
            return false;
    }

    return true;
}

VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};

    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.pfnUserCallback = debug_callback;
    debugMessengerCreateInfo.pUserData = nullptr;

    return debugMessengerCreateInfo;
}

void createDebugMessenger(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger)
{
    auto debugMessengerCreateInfo = getDebugMessengerCreateInfo();

    auto createDebugUtilsMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (createDebugUtilsMessengerFunc == nullptr)
        log_error("Failed to load function pointer for vkCreateDebugUtilsMessengerEXT");

    if (createDebugUtilsMessengerFunc(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        log_error("Failed to set up debug messenger");
}

VkImageView createImageView(const DeviceManager& device_manager, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipMapLevels )
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipMapLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device_manager.logicalDevice, &createInfo, nullptr, &image_view) != VK_SUCCESS)
        log_error("Failed to create swapchain image views!");

    return image_view;
}

void VulkanInstance::init()
{
    // Init the window
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "skin-test";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "custom";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    if (!checkValidationLayerSupport())
        log_error("Validation layers not supported!");

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfoForInstance{};
    if (enable_validation_layers)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        instanceCreateInfo.ppEnabledLayerNames = validation_layers.data();

        debugMessengerCreateInfoForInstance = getDebugMessengerCreateInfo();
        instanceCreateInfo.pNext = &debugMessengerCreateInfoForInstance;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
        log_error("vkCreateInstance failed");

    if (enable_validation_layers)
        createDebugMessenger(instance, debugMessenger);

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        log_error("Failed to create window surface");

    device_manager.init(instance, surface);
    swapchain.init(device_manager, window, surface);
    pipeline.init(device_manager, swapchain.imageFormat, swapchain.extent, device_manager.depth_format);
}

void VulkanInstance::deinit()
{
    pipeline.deinit(device_manager);
    swapchain.deinit(device_manager);

    if (enable_validation_layers)
    {
        auto destroyDebugUtilsMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessengerFunc == nullptr)
            log_error("Failed to load function pointer for vkDestroyDebugUtilsMessengerEXT");

        destroyDebugUtilsMessengerFunc(instance, debugMessenger, nullptr);
    }

    device_manager.deinit();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

struct CandidateDeviceSettings
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    VkFormat depth_format = VK_FORMAT_UNDEFINED;

    bool suitable = false;
};

CandidateDeviceSettings isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    CandidateDeviceSettings deviceSettings;

    // basic device properties
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    // optional features support (e.g. texture compression, multi viewport rendering)
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    {
        // find queue families
        uint32_t queueFamilyCount{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, {});
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (int i = 0; i < queueFamilies.size(); ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                deviceSettings.graphicsFamily = i;

            VkBool32 presentSupport{};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                deviceSettings.presentFamily = i;

            if (!deviceSettings.graphicsFamily || !deviceSettings.presentFamily)
                return deviceSettings;
        }
    }

    {
        // check device extension support
        uint32_t extensionCount{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());
        for (const auto& extension : availableExtensions)
            requiredExtensions.erase(extension.extensionName);

        if (!requiredExtensions.empty())
            return deviceSettings;
    }

    {
        // check swap chain support
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &deviceSettings.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, {});
        if (formatCount == 0)
            return deviceSettings;

        deviceSettings.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, deviceSettings.formats.data());

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, {});
        if (presentModeCount == 0)
            return deviceSettings;
         
        deviceSettings.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, deviceSettings.presentModes.data());
    }

    {
        // check depth format support
        constexpr std::array<VkFormat, 3> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device, format, &props);

            if (VK_IMAGE_TILING_OPTIMAL == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                deviceSettings.depth_format = format;
                break;
            }
            else if (VK_IMAGE_TILING_OPTIMAL == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                deviceSettings.depth_format = format;
                break;
            }
        }

        if (deviceSettings.depth_format == VK_FORMAT_UNDEFINED)
            return deviceSettings;
    }

    deviceSettings.suitable = true;
    return deviceSettings;
};

void DeviceManager::init(VkInstance& instance, VkSurfaceKHR& surface)
{
    {
        // select physical device
        uint32_t deviceCount{};
        vkEnumeratePhysicalDevices(instance, &deviceCount, {});

        if (!deviceCount)
            log_error("failed to find gpus with Vulkan support!");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // TODO: pick best device instead of just first
        for (auto device : devices)
        {
            auto deviceSettings = isDeviceSuitable(device, surface);
            if (deviceSettings.suitable)
            {
                physicalDevice = device;
                graphicsQueueFamily = deviceSettings.graphicsFamily.value();
                presentQueueFamily = deviceSettings.presentFamily.value();

                surface_capabilities = deviceSettings.capabilities;
                surface_formats = deviceSettings.formats;
                surface_presentModes = deviceSettings.presentModes;
                depth_format = deviceSettings.depth_format;
                
                msaaSamples = [&device]() {
                    VkPhysicalDeviceProperties physicalDeviceProperties;
                    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

                    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
                    if (counts & VK_SAMPLE_COUNT_64_BIT)
                        return VK_SAMPLE_COUNT_64_BIT;
                    else if (counts & VK_SAMPLE_COUNT_32_BIT)
                        return VK_SAMPLE_COUNT_32_BIT;
                    else if (counts & VK_SAMPLE_COUNT_16_BIT)
                        return VK_SAMPLE_COUNT_16_BIT;
                    else if (counts & VK_SAMPLE_COUNT_8_BIT)
                        return VK_SAMPLE_COUNT_8_BIT;
                    else if (counts & VK_SAMPLE_COUNT_4_BIT)
                        return VK_SAMPLE_COUNT_4_BIT;
                    else if (counts & VK_SAMPLE_COUNT_2_BIT)
                        return VK_SAMPLE_COUNT_2_BIT;

                    return VK_SAMPLE_COUNT_1_BIT;
                }();

                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
            log_error("failed to find suitable gpu");
    }

    {
        // create logical device
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamily, presentQueueFamily };

        const float queuePriority = 1.0f;
        for (auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading to stop alisaing within textures

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = (uint32_t)device_extensions.size();
        createInfo.ppEnabledExtensionNames = device_extensions.data();

        // only valid for older versions of vulkan
        if (enable_validation_layers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            createInfo.ppEnabledLayerNames = validation_layers.data();
        }
        else
            createInfo.enabledLayerCount = 0;
       
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        if (vkCreateDevice(physicalDevice, &createInfo, {}, &logicalDevice) != VK_SUCCESS)
            log_error("failed to create logical device!");

        vkGetDeviceQueue(logicalDevice, graphicsQueueFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, presentQueueFamily, 0, &presentQueue);
    }
}

void DeviceManager::deinit()
{
    vkDestroyDevice(logicalDevice, nullptr);
}

void Swapchain::init(const DeviceManager& device_manager, GLFWwindow* window, VkSurfaceKHR surface)
{
    // Select a surface format
    if (device_manager.surface_formats.empty()) log_error("No surface formats");

    VkSurfaceFormatKHR surfaceFormat = device_manager.surface_formats[0];
    for (const auto& availableFormat : device_manager.surface_formats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            surfaceFormat = availableFormat;
    }

    // Select a present mode
    if (device_manager.surface_presentModes.empty()) log_error("No present modes");

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availableMode : device_manager.surface_presentModes)
    {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
            presentMode = availableMode;
    }

    // Work out the extent
    {
        if (device_manager.surface_capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = device_manager.surface_capabilities.currentExtent;
        }
        else
        {
            int framebuffer_width;
            int framebuffer_height;
            glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

            VkExtent2D actualExtent = { static_cast<uint32_t>(framebuffer_width), static_cast<uint32_t>(framebuffer_height) };
            actualExtent.width = std::clamp(actualExtent.width, device_manager.surface_capabilities.minImageExtent.width, device_manager.surface_capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, device_manager.surface_capabilities.minImageExtent.height, device_manager.surface_capabilities.maxImageExtent.height);
            
            extent = actualExtent;
        }
    }

    uint32_t image_count = device_manager.surface_capabilities.minImageCount + 1;
    if (device_manager.surface_capabilities.maxImageCount > 0)
        image_count = std::min(image_count, device_manager.surface_capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = image_count;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[] = { device_manager.graphicsQueueFamily, device_manager.presentQueueFamily };
    if (device_manager.graphicsQueueFamily != device_manager.presentQueueFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = device_manager.surface_capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_manager.logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
        log_error("Failed to create swapchain!");

    vkGetSwapchainImagesKHR(device_manager.logicalDevice, handle, &image_count, nullptr);
    swapChainImages.resize(image_count);
    vkGetSwapchainImagesKHR(device_manager.logicalDevice, handle, &image_count, swapChainImages.data());

    swapChainImageViews.resize(image_count);
    for (size_t i = 0; i < image_count; i++)
    {
        swapChainImageViews[i] = createImageView(device_manager, swapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    imageFormat = surfaceFormat.format;
}

void Swapchain::deinit(const DeviceManager& device_manager)
{
    for (auto imageView : swapChainImageViews)
        vkDestroyImageView(device_manager.logicalDevice, imageView, nullptr);

    vkDestroySwapchainKHR(device_manager.logicalDevice, handle, nullptr);
}

struct Shader
{
    enum class Type {
        Vertex,
        Fragment
    };

    VkShaderModule shader_module{};
    VkPipelineShaderStageCreateInfo create_info{};

    void init(const std::string& file_path, const Type type, VkDevice logical_device)
    {
        // read file
        std::vector<char> buffer;
        {
            std::ifstream file(file_path, std::ios::ate | std::ios::binary);

            if (!file.is_open())
                log_error("failed to open file!");

            size_t fileSize = (size_t)file.tellg();
            buffer.resize(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
        }

        // create shader module
        {
            VkShaderModuleCreateInfo module_create_info{};
            module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            module_create_info.codeSize = buffer.size();
            module_create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

            if (vkCreateShaderModule(logical_device, &module_create_info, nullptr, &shader_module) != VK_SUCCESS)
                log_error("failed to create shader module!");
        }

        // create shader stage
        {
            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage = type == Type::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
            create_info.module = shader_module;
            create_info.pName = "main";
        }
    }

    void deinit(VkDevice logical_device)
    {
        vkDestroyShaderModule(logical_device, shader_module, nullptr);
    }
};

void Pipeline::init(const DeviceManager& device_manager, VkFormat swapchain_format, VkExtent2D swapchain_extent, VkFormat depth_format)
{
    // Render pass
    {
        std::array<VkAttachmentDescription, 3> attachment_descriptions = {};
        std::array<VkAttachmentReference, 3> attachment_references = {};

        // Colour
        VkAttachmentDescription& colourAttachment = attachment_descriptions[0];
        colourAttachment.format = swapchain_format;
        colourAttachment.samples = device_manager.msaaSamples;
        colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference& colourAttachmentRef = attachment_references[0];
        colourAttachmentRef.attachment = 0;
        colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth
        VkAttachmentDescription& depthAttachment = attachment_descriptions[1];
        depthAttachment.format = depth_format;
        depthAttachment.samples = device_manager.msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference& depthAttachmentRef = attachment_references[1];
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Colour resolve
        VkAttachmentDescription& resolveAttachment = attachment_descriptions[2];
        resolveAttachment.format = swapchain_format;
        resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference& resolveAttachmentRef = attachment_references[2];
        resolveAttachmentRef.attachment = 2;
        resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colourAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &resolveAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 3;
        renderPassInfo.pAttachments = attachment_descriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device_manager.logicalDevice, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS)
            log_error("Failed to create render pass");
    }

    // create descriptor set
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device_manager.logicalDevice, &layoutInfo, nullptr, &descriptor_set_layout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout");
    }

    // create graphics pipeline 
    {
        Shader vert_shader;
        Shader frag_shader;
        vert_shader.init("../Shaders/vert.spv", Shader::Type::Vertex, device_manager.logicalDevice);
        frag_shader.init("../Shaders/frag.spv", Shader::Type::Fragment, device_manager.logicalDevice);

        VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader.create_info, frag_shader.create_info };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        const auto bindingDescription = Vertex::getBindingDescription();
        const auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain_extent.width;
        viewport.height = (float)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0,0 };
        scissor.extent = swapchain_extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = device_manager.msaaSamples;
        multisampling.minSampleShading = 0.2f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colourBlendAttachment{};
        colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colourBlendAttachment.blendEnable = VK_TRUE;
        colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colourBlending{};
        colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colourBlending.logicOpEnable = VK_FALSE;
        colourBlending.logicOp = VK_LOGIC_OP_COPY;
        colourBlending.attachmentCount = 1;
        colourBlending.pAttachments = &colourBlendAttachment;
        colourBlending.blendConstants[0] = 0.0f;
        colourBlending.blendConstants[1] = 0.0f;
        colourBlending.blendConstants[2] = 0.0f;
        colourBlending.blendConstants[3] = 0.0f;

        //VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

        //VkPipelineDynamicStateCreateInfo dynamicState{};
        //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        //dynamicState.dynamicStateCount = 2;
        //dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device_manager.logicalDevice, &pipelineLayoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS)
            log_error("failed to create pipeline layout!");

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 0.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colourBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.layout = pipeline_layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device_manager.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS)
            log_error("failed to create graphics pipeline!");

        vert_shader.deinit(device_manager.logicalDevice);
        frag_shader.deinit(device_manager.logicalDevice);
    }
}

void Pipeline::deinit(const DeviceManager& device_manager)
{
    vkDestroyPipeline(device_manager.logicalDevice, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device_manager.logicalDevice, pipeline_layout, nullptr);
    vkDestroyRenderPass(device_manager.logicalDevice, render_pass, nullptr);

    vkDestroyDescriptorSetLayout(device_manager.logicalDevice, descriptor_set_layout, nullptr);
}