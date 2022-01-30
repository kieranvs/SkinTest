#include "VulkanInstance.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include <iostream>

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

    for (int i = 0; i < glfwExtensionCount; i++)
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
    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}