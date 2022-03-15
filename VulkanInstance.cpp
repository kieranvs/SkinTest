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
const int MAX_FRAMES_IN_FLIGHT = 2;

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

uint32_t findMemoryType(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
    for (uint32_t i{}; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    log_error("failed to find suitable memory type!");
}

void Image::createImage(
    VkDevice logical_device,
    VkPhysicalDevice physical_device,
    const uint32_t width,
    const uint32_t height,
    uint32_t mipMapLevels,
    VkSampleCountFlagBits numSamples,
    const VkFormat format,
    const VkImageTiling tiling,
    const VkImageUsageFlags usage)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipMapLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0;

    if (vkCreateImage(logical_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        log_error("failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logical_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &image_memory) != VK_SUCCESS)
        log_error("failed to allocate image memory");

    vkBindImageMemory(logical_device, image, image_memory, 0);
}

void Image::deinit(VkDevice logical_device) {
    vkDestroyImageView(logical_device, image_view, nullptr);
    vkDestroyImage(logical_device, image, nullptr);
    vkFreeMemory(logical_device, image_memory, nullptr);
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
    pipeline.init(device_manager, swapchain);

    // Create sync objects
    {
        image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
        images_in_flight.resize(swapchain.swapChainImages.size(), VK_NULL_HANDLE);

        // similar to fences but can only be used within or across queues
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (vkCreateSemaphore(device_manager.logicalDevice, &semaphoreInfo, nullptr, &image_available_semaphores[i]) != VK_SUCCESS)
                log_error("failed to create synchronisation objects for a frame!");

            if (vkCreateSemaphore(device_manager.logicalDevice, &semaphoreInfo, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS)
                log_error("failed to create synchronisation objects for a frame!");

            if (vkCreateFence(device_manager.logicalDevice, &fenceInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
                log_error("failed to create synchronisation objects for a frame!");
        }
    }
}

void VulkanInstance::deinit()
{
    vkFreeCommandBuffers(device_manager.logicalDevice, device_manager.command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());

    pipeline.deinit(device_manager);
    swapchain.deinit(device_manager);

    vertex_buffer.deinit(device_manager.logicalDevice);
    index_buffer.deinit(device_manager.logicalDevice);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device_manager.logicalDevice, render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(device_manager.logicalDevice, image_available_semaphores[i], nullptr);
        vkDestroyFence(device_manager.logicalDevice, in_flight_fences[i], nullptr);
    }

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

void SingleTimeCommandBuffer::begin(VkDevice logical_device, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(logical_device, &allocInfo, &command_buffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &beginInfo);
}

void SingleTimeCommandBuffer::end(VkDevice logical_device, VkQueue graphics_queue, VkCommandPool command_pool)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
}

void Buffer::init(VkPhysicalDevice physical_device, VkDevice logical_device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
{
    buffer_size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(logical_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        log_error("failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logical_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(logical_device, &allocInfo, nullptr, &buffer_memory) != VK_SUCCESS)
        log_error("failed to allocate buffer memory!");

    vkBindBufferMemory(logical_device, buffer, buffer_memory, 0);
}

void Buffer::deinit(VkDevice logical_device)
{
    vkDestroyBuffer(logical_device, buffer, nullptr);
    vkFreeMemory(logical_device, buffer_memory, nullptr);
}

void copyBuffer(VkDevice logical_device, VkCommandPool command_pool, VkQueue graphics_queue, Buffer& src_buffer, Buffer& dst_buffer)
{
    if (src_buffer.buffer_size != dst_buffer.buffer_size)
        log_error("Mismatched buffer size!");

    SingleTimeCommandBuffer command_buffer;
    command_buffer.begin(logical_device, command_pool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = src_buffer.buffer_size;
    vkCmdCopyBuffer(command_buffer.command_buffer, src_buffer.buffer, dst_buffer.buffer, 1, &copyRegion);

    command_buffer.end(logical_device, graphics_queue, command_pool);
}

template<typename T>
void uploadData(Buffer& buffer, VkDevice logical_device, const std::vector<T>& data)
{
    void* ptr;
    vkMapMemory(logical_device, buffer.buffer_memory, 0, buffer.buffer_size, 0, &ptr);
    memcpy(ptr, data.data(), (size_t)buffer.buffer_size);
    vkUnmapMemory(logical_device, buffer.buffer_memory);
}

template<typename T>
void uploadBufferData(const DeviceManager& device_manager, Buffer& buffer, const std::vector<T>& data, VkBufferUsageFlagBits usage)
{
    const VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

    Buffer stagingBuffer;
    stagingBuffer.init(device_manager.physicalDevice, device_manager.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uploadData(stagingBuffer, device_manager.logicalDevice, data);

    buffer.init(device_manager.physicalDevice, device_manager.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffer.num_elements = data.size();

    copyBuffer(device_manager.logicalDevice, device_manager.command_pool, device_manager.graphicsQueue, stagingBuffer, buffer);

    stagingBuffer.deinit(device_manager.logicalDevice);
}

void VulkanInstance::createBuffers(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    // create vertex buffer
    uploadBufferData(device_manager, vertex_buffer, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    // create index buffer
    uploadBufferData(device_manager, index_buffer, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void VulkanInstance::createCommandBuffers()
{
    command_buffers.resize(pipeline.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device_manager.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

    if (vkAllocateCommandBuffers(device_manager.logicalDevice, &allocInfo, command_buffers.data()) != VK_SUCCESS)
        log_error("failed to allocate command buffers!");

    for (size_t i = 0; i < command_buffers.size(); ++i)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(command_buffers[i], &beginInfo) != VK_SUCCESS)
            log_error("failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pipeline.render_pass;
        renderPassInfo.framebuffer = pipeline.framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain.extent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // no error handling from here while recording
        vkCmdBeginRenderPass(command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphics_pipeline);

        VkBuffer vertexBuffers[] = { vertex_buffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &pipeline.descriptor_sets[i], 0, nullptr);

        vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(index_buffer.num_elements), 1, 0, 0, 0);

        vkCmdEndRenderPass(command_buffers[i]);

        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
            log_error("failed to record command buffer!");
    }
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

    // create command pool
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = graphicsQueueFamily;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &command_pool) != VK_SUCCESS)
            log_error("failed to create command pool!");
    }
}

void DeviceManager::deinit()
{
    vkDestroyCommandPool(logicalDevice, command_pool, nullptr);

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

void Pipeline::init(const DeviceManager& device_manager, const Swapchain& swapchain)
{
    // Render pass
    {
        std::array<VkAttachmentDescription, 3> attachment_descriptions = {};
        std::array<VkAttachmentReference, 3> attachment_references = {};

        // Colour
        VkAttachmentDescription& colourAttachment = attachment_descriptions[0];
        colourAttachment.format = swapchain.imageFormat;
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
        depthAttachment.format = device_manager.depth_format;
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
        resolveAttachment.format = swapchain.imageFormat;
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

        // VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        // samplerLayoutBinding.binding = 1;
        // samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // samplerLayoutBinding.descriptorCount = 1;
        // samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // samplerLayoutBinding.pImmutableSamplers = nullptr;

        // std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device_manager.logicalDevice, &layoutInfo, nullptr, &descriptor_set_layout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout");
    }

    // create descriptor pool
    {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain.swapChainImages.size());
        // poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain.swapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapchain.swapChainImages.size());

        if (vkCreateDescriptorPool(device_manager.logicalDevice, &poolInfo, nullptr, &descriptor_pool) != VK_SUCCESS)
            log_error("failed to create descriptor pool");
    }

    // Create uniform buffers
    {
        VkDeviceSize bufferSize = sizeof(UniformData);

        uniform_buffers.resize(swapchain.swapChainImages.size());

        for (auto& buffer : uniform_buffers)
        {
            buffer.init(device_manager.physicalDevice, device_manager.logicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    // create descriptor sets
    {
        std::vector<VkDescriptorSetLayout> layouts(swapchain.swapChainImages.size(), descriptor_set_layout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptor_pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptor_sets.resize(swapchain.swapChainImages.size());
        if (vkAllocateDescriptorSets(device_manager.logicalDevice, &allocInfo, descriptor_sets.data()) != VK_SUCCESS)
            log_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < swapchain.swapChainImages.size(); ++i)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniform_buffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformData);

            // VkDescriptorImageInfo imageInfo{};
            // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            // imageInfo.imageView = texture.image_view;
            // imageInfo.sampler = texture.sampler;

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptor_sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            // descriptorWrites[1].dstSet = descriptor_sets[i];
            // descriptorWrites[1].dstBinding = 1;
            // descriptorWrites[1].dstArrayElement = 0;
            // descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            // descriptorWrites[1].descriptorCount = 1;
            // descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device_manager.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
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
        viewport.width = (float)swapchain.extent.width;
        viewport.height = (float)swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0,0 };
        scissor.extent = swapchain.extent;

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

    // create colour and depth resources
    {
        colour_image.createImage(
            device_manager.logicalDevice,
            device_manager.physicalDevice,
            swapchain.extent.width,
            swapchain.extent.height,
            1,
            device_manager.msaaSamples,
            swapchain.imageFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );
        colour_image.image_view = createImageView(device_manager, colour_image.image, swapchain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        
        depth_image.createImage(
            device_manager.logicalDevice,
            device_manager.physicalDevice,
            swapchain.extent.width,
            swapchain.extent.height,
            1,
            device_manager.msaaSamples,
            device_manager.depth_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
        depth_image.image_view = createImageView(device_manager, depth_image.image, device_manager.depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    // create framebuffers
    {
        framebuffers.resize(swapchain.swapChainImageViews.size());

        for (size_t i{}; i < swapchain.swapChainImageViews.size(); ++i)
        {
            std::array<VkImageView, 3> attachments = { colour_image.image_view, depth_image.image_view, swapchain.swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render_pass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchain.extent.width;
            framebufferInfo.height = swapchain.extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device_manager.logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
                log_error("Failed to create framebuffer");
        }
    }
}

void Pipeline::deinit(const DeviceManager& device_manager)
{
    for (auto& buffer : uniform_buffers)
        buffer.deinit(device_manager.logicalDevice);

    colour_image.deinit(device_manager.logicalDevice);
    depth_image.deinit(device_manager.logicalDevice);

    for (auto& framebuffer : framebuffers)
        vkDestroyFramebuffer(device_manager.logicalDevice, framebuffer, nullptr);

    vkDestroyPipeline(device_manager.logicalDevice, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device_manager.logicalDevice, pipeline_layout, nullptr);
    vkDestroyRenderPass(device_manager.logicalDevice, render_pass, nullptr);

    vkDestroyDescriptorSetLayout(device_manager.logicalDevice, descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(device_manager.logicalDevice, descriptor_pool, nullptr);
}