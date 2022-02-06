#include "VulkanInstance.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>

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
}

void VulkanInstance::deinit()
{
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
