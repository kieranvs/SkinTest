#pragma once

#include "VulkanInstance.h"

struct ImguiImpl
{
	VkDescriptorPool descriptor_pool;
	VkRenderPass render_pass;
	CommandBufferSet command_buffer_set;
	std::vector<VkFramebuffer> framebuffers;

	void init(VulkanInstance& instance);
	void deinit(VulkanInstance& instance);

	void swapchainRecreate(VulkanInstance& instance);
	void renderFrame(VulkanInstance& instance, size_t frame_index);
};
