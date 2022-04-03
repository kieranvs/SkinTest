#include "Shader.h"

#include "Log.h"

#include <vector>
#include <fstream>

namespace VulkanWrapper
{
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

            if (vkCreateShaderModule(logical_device, &module_create_info, nullptr, &shader_module) != VK_SUCCESS)
                log_error("failed to create shader module!");
        }

        // create shader stage
        {
            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage = type == Type::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
            create_info.module = shader_module;
            create_info.pName = "main";
        }
    }

    void Shader::deinit(VkDevice logical_device)
    {
        vkDestroyShaderModule(logical_device, shader_module, nullptr);
    }
}