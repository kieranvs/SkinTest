#pragma once

#include "VulkanInstance.h"

struct ImguiImpl
{
	VkDescriptorPool descriptor_pool;
	VkRenderPass render_pass;

	void init(VulkanInstance& instance);
	void deinit(VulkanInstance& instance);
};
