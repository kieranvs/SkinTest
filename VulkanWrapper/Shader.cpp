#include "Shader.h"

#include "DeviceManager.h"
#include "Log.h"

#include <vector>
#include <fstream>

namespace VulkanWrapper
{
    void DescriptorSetLayout::upload(DeviceManager& device_manager)
    {
        if (bindings.empty())
            log_error("DescriptorSetLayoutInfo bindings not set");

        std::vector<VkDescriptorSetLayoutBinding> vkbindings(bindings.size());
        for (uint32_t binding_index = 0; binding_index < bindings.size(); binding_index++)
        {
            auto& lb = vkbindings[binding_index];

            lb.binding = binding_index;
            lb.descriptorType = bindings[binding_index].descriptor_type;
            lb.descriptorCount = 1;
            lb.stageFlags = bindings[binding_index].stage_flags;
            lb.pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(vkbindings.size());
        layoutInfo.pBindings = vkbindings.data();

        if (vkCreateDescriptorSetLayout(device_manager.logicalDevice, &layoutInfo, nullptr, &handle) != VK_SUCCESS)
            log_error("failed to create descriptor set layout");
    }

    void DescriptorSetLayout::deinit(DeviceManager& device_manager)
    {
        vkDestroyDescriptorSetLayout(device_manager.logicalDevice, handle, nullptr);
    }

    void Shader::init(const std::string& file_path, const Type type, VkDevice logical_device)
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

            if (vkCreateShaderModule(logical_device, &module_create_info, nullptr, &handle) != VK_SUCCESS)
                log_error("failed to create shader module!");
        }

        // create shader stage
        {
            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage = type == Type::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
            create_info.module = handle;
            create_info.pName = "main";
        }
    }

    void Shader::deinit(VkDevice logical_device)
    {
        vkDestroyShaderModule(logical_device, handle, nullptr);
    }
}