#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace VulkanWrapper
{
    struct Shader
    {
        enum class Type {
            Vertex,
            Fragment
        };

        VkShaderModule shader_module{};
        VkPipelineShaderStageCreateInfo create_info{};

        void init(const std::string& file_path, const Type type, VkDevice logical_device);
        void deinit(VkDevice logical_device);
    };
}