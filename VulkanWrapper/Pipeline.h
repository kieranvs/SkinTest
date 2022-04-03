#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWrapper/Image.h"
#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/DeviceManager.h"
#include "VulkanWrapper/Swapchain.h"

namespace VulkanWrapper
{
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

        void init(const DeviceManager& device_manager, const Swapchain& swapchain, VkDeviceSize uniform_size);
        void deinit(const DeviceManager& device_manager);
    };
}