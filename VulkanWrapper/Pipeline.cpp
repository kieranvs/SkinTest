#include "Pipeline.h"

#include "Shader.h"
#include "Log.h"

#include <array>
#include <stdexcept>

namespace VulkanWrapper
{
    void Pipeline::init(const DeviceManager& device_manager, const Swapchain& swapchain, const ShaderSettings& shader_settings)
    {
        // Render pass
        {
            std::array<VkAttachmentDescription, 3> attachment_descriptions = {};
            std::array<VkAttachmentReference, 3> attachment_references = {};

            // Colour
            VkAttachmentDescription& colourAttachment = attachment_descriptions[0];
            colourAttachment.format = swapchain.image_format;
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
            resolveAttachment.format = swapchain.image_format;
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
            poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain.images.size());
            // poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            // poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain.swapChainImages.size());

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = static_cast<uint32_t>(swapchain.images.size());

            if (vkCreateDescriptorPool(device_manager.logicalDevice, &poolInfo, nullptr, &descriptor_pool) != VK_SUCCESS)
                log_error("failed to create descriptor pool");
        }

        // Create uniform buffers
        {
            uniform_buffers.resize(swapchain.images.size());

            for (auto& buffer : uniform_buffers)
            {
                buffer.init(device_manager.physicalDevice, device_manager.logicalDevice, shader_settings.uniform_data_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }

        // create descriptor sets
        {
            std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), descriptor_set_layout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptor_pool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.images.size());
            allocInfo.pSetLayouts = layouts.data();

            descriptor_sets.resize(swapchain.images.size());
            if (vkAllocateDescriptorSets(device_manager.logicalDevice, &allocInfo, descriptor_sets.data()) != VK_SUCCESS)
                log_error("failed to allocate descriptor sets!");

            for (size_t i = 0; i < swapchain.images.size(); ++i)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniform_buffers[i].handle;
                bufferInfo.offset = 0;
                bufferInfo.range = shader_settings.uniform_data_size;

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
            vert_shader.init(shader_settings.vert_addr, Shader::Type::Vertex, device_manager.logicalDevice);
            frag_shader.init(shader_settings.frag_addr, Shader::Type::Fragment, device_manager.logicalDevice);

            VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader.create_info, frag_shader.create_info };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &shader_settings.binding_description;
            vertexInputInfo.vertexAttributeDescriptionCount = shader_settings.input_attribute_descriptions_count;
            vertexInputInfo.pVertexAttributeDescriptions = shader_settings.input_attribute_descriptions;

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
                swapchain.image_format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            );
            colour_image.view = createImageView(device_manager.logicalDevice, colour_image.handle, swapchain.image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            
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
            depth_image.view = createImageView(device_manager.logicalDevice, depth_image.handle, device_manager.depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        }

        // create framebuffers
        {
            framebuffers.resize(swapchain.image_views.size());

            for (size_t i{}; i < swapchain.image_views.size(); ++i)
            {
                std::array<VkImageView, 3> attachments = { colour_image.view, depth_image.view, swapchain.image_views[i] };

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
}