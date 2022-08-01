#include "ImguiImpl.h"

#include <VulkanWrapper/Log.h>

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

void check_vk_result(VkResult err)
{
	if (err != VK_SUCCESS)
		log_error("Imgui failure");
}

VkRenderPass createRenderPass(VkDevice logical_device, VkFormat swapchain_format)
{
	VkAttachmentDescription attachment = {};
	attachment.format = swapchain_format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	VkRenderPass imGuiRenderPass;
	if (vkCreateRenderPass(logical_device, &info, nullptr, &imGuiRenderPass) != VK_SUCCESS)
	{
	    log_error("Could not create Dear ImGui's render pass");
	}

	return imGuiRenderPass;
}

VkDescriptorPool createDescriptorPool(VkDevice logical_device)
{
	std::vector<VkDescriptorPoolSize> pool_sizes =
	{
	    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
	    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
	    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
	    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pool_sizes.size();
    poolInfo.pPoolSizes = pool_sizes.data();
    poolInfo.maxSets = 1000;

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(logical_device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        log_error("failed to create descriptor pool");

    return pool;
}

void createFramebuffers(VulkanInstance& instance, std::vector<VkFramebuffer>& framebuffers, VkRenderPass render_pass)
{
    VkImageView attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = render_pass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = instance.swapchain.extent.width;
    info.height = instance.swapchain.extent.height;
    info.layers = 1;

    framebuffers.resize(instance.swapchain.images.size());
    for (uint32_t i = 0; i < instance.swapchain.images.size(); i++)
    {
        attachment[0] = instance.swapchain.images[i].view;
        if (vkCreateFramebuffer(instance.device_manager.logicalDevice, &info, nullptr, &framebuffers[i]) != VK_SUCCESS)
        	log_error("Failed to create imgui framebuffers");
    }
}

void ImguiImpl::init(VulkanInstance& instance)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	descriptor_pool = createDescriptorPool(instance.device_manager.logicalDevice);

	ImGui_ImplGlfw_InitForVulkan(instance.window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance.instance;
	init_info.PhysicalDevice = instance.device_manager.physicalDevice;
	init_info.Device = instance.device_manager.logicalDevice;
	init_info.QueueFamily = instance.device_manager.graphicsQueueFamily;
	init_info.Queue = instance.device_manager.graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = descriptor_pool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = instance.swapchain.images.size();
	init_info.ImageCount = instance.swapchain.images.size();
	init_info.CheckVkResultFn = nullptr;

	render_pass = createRenderPass(instance.device_manager.logicalDevice, instance.swapchain.image_format);
	ImGui_ImplVulkan_Init(&init_info, render_pass);

	createFramebuffers(instance, framebuffers, render_pass);

	SingleTimeCommandBuffer command_buffer;
	command_buffer.begin(instance.device_manager);
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer.getHandle());
	command_buffer.end(instance.device_manager);

	command_buffer_set.init(instance.device_manager, init_info.ImageCount);
}

void ImguiImpl::deinit(VulkanInstance& instance)
{
	for (size_t i = 0; i < framebuffers.size(); i++)
		vkDestroyFramebuffer(instance.device_manager.logicalDevice, framebuffers[i], nullptr);
	vkDestroyRenderPass(instance.device_manager.logicalDevice, render_pass, nullptr);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	
	vkDestroyDescriptorPool(instance.device_manager.logicalDevice, descriptor_pool, nullptr);
}

void ImguiImpl::swapchainRecreate(VulkanInstance& instance)
{
	ImGui_ImplVulkan_SetMinImageCount(instance.swapchain.images.size());

	command_buffer_set.deinit(instance.device_manager);
	command_buffer_set.init(instance.device_manager, instance.swapchain.images.size());
}

void ImguiImpl::renderFrame(VulkanInstance& instance, size_t frame_index)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();

	auto draw_data = ImGui::GetDrawData();

	command_buffer_set.begin(frame_index, 0);

	VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = framebuffers[frame_index];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = instance.swapchain.extent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command_buffer_set[frame_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
	ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer_set[frame_index]);
	
    vkCmdEndRenderPass(command_buffer_set[frame_index]);

	command_buffer_set.end();
}