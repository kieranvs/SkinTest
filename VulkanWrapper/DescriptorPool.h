#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/Shader.h"

#include <vector>

namespace VulkanWrapper {
	struct DescriptorPool {
		VkDescriptorPool handle;

		std::vector<VkDescriptorSet> createDescriptorSets(VkDevice logical_device, const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<Buffer>& uniform_buffers,Image& texture, VkSampler sampler, const std::vector<VulkanWrapper::UniformBufferBinding>& uniform_buffer_bindings);

		void init(VkDevice logical_device, uint32_t uniform_buffer_count, uint32_t sampler_count, uint32_t set_count);
		void deinit(VkDevice logical_device);
	};
}