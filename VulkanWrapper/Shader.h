#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace VulkanWrapper
{
    class DeviceManager;

    struct DescriptorSetLayoutInfo
    {
        VkDescriptorType descriptor_type;
        VkShaderStageFlags stage_flags;
        VkDeviceSize uniform_data_size;
        uint32_t count;
    };

    struct DescriptorSetLayout
    {
        std::vector<DescriptorSetLayoutInfo> bindings;
        bool update_per_frame = false;

        VkDescriptorSetLayout handle;

        void upload(DeviceManager& device_manager);
        void deinit(DeviceManager& device_manager);
    };

    struct ShaderSettings
    {
        const char* vert_addr;
        const char* frag_addr;

        VkVertexInputBindingDescription binding_description;
        const VkVertexInputAttributeDescription* input_attribute_descriptions;
        uint32_t input_attribute_descriptions_count;

        std::vector<DescriptorSetLayout> descriptor_set_layouts;
    };
    
    struct Shader
    {
        enum class Type {
            Vertex,
            Fragment
        };

        VkShaderModule handle{};
        VkPipelineShaderStageCreateInfo create_info{};

        void init(const std::string& file_path, const Type type, VkDevice logical_device);
        void deinit(VkDevice logical_device);
    };
}