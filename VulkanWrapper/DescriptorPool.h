#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/Shader.h"

#include <vector>

namespace VulkanWrapper
{
	struct Texture;

	struct DescriptorPool
	{
		VkDescriptorPool handle;

		VkDescriptorSet createDescriptorSet(VkDevice logical_device, const DescriptorSetLayout& layout, const std::vector<Buffer*>& uniform_buffers, const std::vector<Texture*>& textures);

		void init(VkDevice logical_device, const uint32_t swapchain_count, const std::vector<DescriptorSetLayout>& descriptor_set_layouts);
		void deinit(VkDevice logical_device);
	};
}