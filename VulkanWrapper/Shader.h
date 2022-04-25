#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace VulkanWrapper
{
    struct UniformBufferBinding
    {
        VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        VkShaderStageFlags stage_flags;
        VkDeviceSize uniform_data_size;
    };

    struct ShaderSettings
    {
        const char* vert_addr;
        const char* frag_addr;

        VkVertexInputBindingDescription binding_description;
        const VkVertexInputAttributeDescription* input_attribute_descriptions;
        uint32_t input_attribute_descriptions_count;

        std::vector<UniformBufferBinding> uniform_bindings;
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