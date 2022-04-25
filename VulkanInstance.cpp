#include "VulkanInstance.h"
#include "VulkanWrapper/Log.h"

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

void VulkanInstance::init()
{
    // Init the window
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
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

    pipeline.init(device_manager, swapchain, shader_settings);

    // Create sync objects
    {
        image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        frame_finished_fences.resize(MAX_FRAMES_IN_FLIGHT);
        image_to_frame_fences.resize(swapchain.images.size(), VK_NULL_HANDLE);

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

            if (vkCreateFence(device_manager.logicalDevice, &fenceInfo, nullptr, &frame_finished_fences[i]) != VK_SUCCESS)
                log_error("failed to create synchronisation objects for a frame!");
        }
    }
}

void VulkanInstance::deinit()
{
    command_buffer_set.deinit(device_manager);

    pipeline.deinit(device_manager);
    swapchain.deinit(device_manager);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device_manager.logicalDevice, render_finished_semaphores[i], nullptr);
        vkDestroySemaphore(device_manager.logicalDevice, image_available_semaphores[i], nullptr);
        vkDestroyFence(device_manager.logicalDevice, frame_finished_fences[i], nullptr);
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

void VulkanInstance::createCommandBuffers()
{
    command_buffer_set.init(device_manager, pipeline.framebuffers.size());

    for (size_t i = 0; i < command_buffer_set.size(); ++i)
    {
        command_buffer_set.begin(i, 0);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pipeline.render_pass;
        renderPassInfo.framebuffer = pipeline.framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapchain.extent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // no error handling from here while recording
        vkCmdBeginRenderPass(command_buffer_set[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer_set[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.graphics_pipeline);

        command_buffer_callback(pipeline, i, command_buffer_set[i]);

        vkCmdEndRenderPass(command_buffer_set[i]);

        command_buffer_set.end();
    }
}

void VulkanInstance::mainLoop()
{
    createCommandBuffers();

    size_t currentFrame = 0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        // image synchronisation
        uint32_t image_index;
        {
            vkWaitForFences(device_manager.logicalDevice, 1, &frame_finished_fences[currentFrame], VK_TRUE, UINT64_MAX);

            VkResult result = vkAcquireNextImageKHR(device_manager.logicalDevice, swapchain.handle, UINT64_MAX, image_available_semaphores[currentFrame], VK_NULL_HANDLE, &image_index);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreateSwapChain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                log_error("failed to acquire swap chain image!");

            // wait if a frame in flight is using this image
            if (image_to_frame_fences[image_index] != VK_NULL_HANDLE)
                vkWaitForFences(device_manager.logicalDevice, 1, &image_to_frame_fences[image_index], VK_TRUE, UINT64_MAX);
            image_to_frame_fences[image_index] = frame_finished_fences[currentFrame];
        }

        // update uniform buffer
        update_uniforms_callback(pipeline.uniform_buffers[image_index], device_manager.logicalDevice);

        // submit command buffer
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { image_available_semaphores[currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &command_buffer_set[image_index];

            VkSemaphore signalSemaphores[] = { render_finished_semaphores[currentFrame] };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            vkResetFences(device_manager.logicalDevice, 1, &frame_finished_fences[currentFrame]);
            if (vkQueueSubmit(device_manager.graphicsQueue, 1, &submitInfo, frame_finished_fences[currentFrame]) != VK_SUCCESS)
                log_error("failed to submit draw command buffers!");
        }

        // present the image
        {
            VkSemaphore waitSemaphores[] = { render_finished_semaphores[currentFrame] };

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = waitSemaphores;

            VkSwapchainKHR swapChains[] = { swapchain.handle };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &image_index;
            presentInfo.pResults = nullptr;

            VkResult result = vkQueuePresentKHR(device_manager.presentQueue, &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
            {
                framebufferResized = false;
                recreateSwapChain();
            }
            else if (result != VK_SUCCESS)
                log_error("failed to present swap chain image!");

            vkQueueWaitIdle(device_manager.presentQueue);
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vkDeviceWaitIdle(device_manager.logicalDevice);
}

void VulkanInstance::recreateSwapChain()
{
    int width{}, height{};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_manager.logicalDevice);

    command_buffer_set.deinit(device_manager);

    pipeline.deinit(device_manager);
    swapchain.deinit(device_manager);

    auto swapchain_support = getSwapchainSupport(device_manager.physicalDevice, surface);
    device_manager.surface_capabilities = swapchain_support.capabilities;
    device_manager.surface_formats = swapchain_support.formats;
    device_manager.surface_presentModes = swapchain_support.present_modes;

    swapchain.init(device_manager, window, surface);
    pipeline.init(device_manager, swapchain, shader_settings);

    createCommandBuffers();
}