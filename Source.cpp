#include "VulkanInstance.h"
#include "Vertex.h"
#include "ModelLoader.h"

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

int main()
{
    VulkanWrapper::Buffer vertex_buffer;
    VulkanWrapper::Buffer index_buffer;
    
    VulkanInstance instance;

    instance.command_buffer_callback = [&vertex_buffer, &index_buffer](const VulkanWrapper::Pipeline& pipeline, const size_t i, const VkCommandBuffer command_buffer) {
        VkBuffer vertexBuffers[] = { vertex_buffer.handle };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(command_buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &pipeline.descriptor_sets[i], 0, nullptr);

        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(index_buffer.count), 1, 0, 0, 0);
    };

    float time_elapsed = 0.0f;

    instance.update_uniforms_callback = [&swapchain = instance.swapchain, &time_elapsed](Buffer& buffer, VkDevice logical_device)
    {
        time_elapsed += 0.016f;

        float x_pos = std::sin(time_elapsed) * 50.0f;
        float z_pos = std::cos(time_elapsed) * 50.0f;

        UniformData uniform_data{};
        uniform_data.model = glm::mat4(1.0f);
        uniform_data.view = glm::lookAt(glm::vec3(x_pos, 20.0f, z_pos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        uniform_data.proj = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 100.0f);
        uniform_data.proj[1][1] *= -1; // correction of inverted Y in OpenGL

        uploadData(buffer, logical_device, &uniform_data);
    };

    instance.init();

    ShaderSettings shader_settings;
    shader_settings.vert_addr = "../Shaders/vert.spv";
    shader_settings.frag_addr = "../Shaders/frag.spv";
    shader_settings.binding_description = Vertex::getBindingDescription();
    const auto& attribute_descriptions = Vertex::getAttributeDescriptions();
    shader_settings.input_attribute_descriptions = attribute_descriptions.data();
    shader_settings.input_attribute_descriptions_count = attribute_descriptions.size();

    // Proj view model mat
    {
        auto& binding = shader_settings.uniform_bindings.emplace_back();
        binding.stage_flags = VK_SHADER_STAGE_VERTEX_BIT;
        binding.uniform_data_size = sizeof(UniformData);
        binding.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    // Texture sampler
    {
        auto& binding = shader_settings.uniform_bindings.emplace_back();
        binding.stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }

    auto [verts, path] = loadModel("../Models/viking_room_gltf/scene.gltf", glm::mat4(1.0f));

    Texture texture;
    texture.init(instance.device_manager, path);

    std::vector<uint32_t> indices{};
    indices.resize(verts.size());
    for (int i = 0; i < verts.size(); ++i)
    {
        indices[i] = i;
    }

    instance.pipeline.init(instance.device_manager, instance.swapchain, shader_settings, &texture);


    uploadBufferData(instance.device_manager, vertex_buffer, verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    uploadBufferData(instance.device_manager, index_buffer, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    instance.mainLoop();

    vertex_buffer.deinit(instance.device_manager.logicalDevice);
    index_buffer.deinit(instance.device_manager.logicalDevice);
    texture.deinit(instance.device_manager.logicalDevice);
    instance.deinit();
}