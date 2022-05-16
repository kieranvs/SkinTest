#include "DescriptorPool.h"

#include "Image.h"
#include "Log.h"

#include <array>

namespace VulkanWrapper
{
	void DescriptorPool::init(VkDevice logical_device, uint32_t uniform_buffer_count, uint32_t sampler_count, uint32_t set_count)
	{
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        uint32_t i = 0;
        if (uniform_buffer_count > 0)
        {
            poolSizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[i].descriptorCount = uniform_buffer_count;
            ++i;
        }
        if (sampler_count > 0)
        {
            poolSizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[i].descriptorCount = sampler_count;
            ++i;
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = i;
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = set_count;

        if (vkCreateDescriptorPool(logical_device, &poolInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("failed to create descriptor pool");
	}

    std::vector<VkDescriptorSet> DescriptorPool::createDescriptorSets(VkDevice logical_device, const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<Buffer>& uniform_buffers, Image& texture, VkSampler sampler, const std::vector<VulkanWrapper::UniformBufferBinding>& uniform_buffer_bindings)
    {
        const uint32_t descriptor_set_count = layouts.size();

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = handle;
        allocInfo.descriptorSetCount = descriptor_set_count;
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptor_sets(descriptor_set_count);
        if (vkAllocateDescriptorSets(logical_device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS)
            log_error("failed to allocate descriptor sets!");

        std::vector<VkDescriptorBufferInfo> buffer_infos(uniform_buffer_bindings.size());
        std::vector<VkDescriptorImageInfo> image_infos(uniform_buffer_bindings.size());
        std::vector<VkWriteDescriptorSet> descriptor_writes(uniform_buffer_bindings.size());

        for (size_t descriptor_set_index = 0; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
        {
            VkDeviceSize current_offset = 0;
            uint32_t current_binding = 0;

            for (uint32_t current_binding = 0; current_binding < uniform_buffer_bindings.size(); current_binding++)
            {
                auto& binding = uniform_buffer_bindings[current_binding];

                auto& descriptor_write = descriptor_writes[current_binding];
                descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_write.dstSet = descriptor_sets[descriptor_set_index];
                descriptor_write.dstBinding = current_binding;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = binding.descriptor_type;
                descriptor_write.descriptorCount = 1;

                if (binding.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                {
                    auto& buffer_info = buffer_infos[current_binding];

                    buffer_info.buffer = uniform_buffers[descriptor_set_index].handle;
                    buffer_info.offset = current_offset;
                    buffer_info.range = binding.uniform_data_size;
                    current_offset += binding.uniform_data_size;

                    descriptor_write.pBufferInfo = &buffer_info;
                }
                else if (binding.descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    auto& image_info = image_infos[current_binding];

                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_info.imageView = texture.view;
                    image_info.sampler = sampler;

                    descriptor_write.pImageInfo = &image_info;
                }
                else
                    log_error("Unsupported uniform binding type");
            }

            vkUpdateDescriptorSets(logical_device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
        }

        return descriptor_sets;
    }

    void DescriptorPool::deinit(VkDevice logical_device)
    {
        vkDestroyDescriptorPool(logical_device, handle, nullptr);
    }
}