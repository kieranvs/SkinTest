#include "VulkanInstance.h"
#include "Vertex.h"
#include "ModelLoader.h"

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

int main()
{
    std::vector<Mesh> meshes;
    std::vector<Buffer> uniform_buffers;
    std::vector<VkDescriptorSet> descriptor_sets;

    VulkanInstance instance{};

    instance.command_buffer_callback = [&meshes, &descriptor_sets](const VulkanWrapper::Pipeline& pipeline, const size_t i, const VkCommandBuffer command_buffer)
    {
        for (size_t m = 0; m < meshes.size(); m++)
        {
            auto& mesh = meshes[m];
            VkBuffer vertexBuffers[] = { mesh.vertex_buffer.handle };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &descriptor_sets[m * pipeline.swapchain_image_size + i], 0, nullptr);

            vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.index_buffer.count), 1, 0, 0, 0);
        }
    };

    auto last_time = glfwGetTime();
    instance.update_uniforms_callback = [&swapchain = instance.swapchain, &last_time, &uniform_buffers](size_t image_index, VkDevice logical_device)
    {
        auto new_time = glfwGetTime();
        auto delta_time = new_time - last_time;
        last_time = new_time;

        float x_pos = std::sin(new_time) * 50.0f;
        float z_pos = std::cos(new_time) * 50.0f;

        UniformData uniform_data{};
        uniform_data.model = glm::mat4(1.0f);
        uniform_data.view = glm::lookAt(glm::vec3(x_pos, 20.0f, z_pos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        uniform_data.proj = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 100.0f);
        uniform_data.proj[1][1] *= -1; // correction of inverted Y in OpenGL

        uploadData(uniform_buffers[image_index], logical_device, &uniform_data);
    };

    instance.init();

    ShaderSettings shader_settings{};
    shader_settings.vert_addr = "../Shaders/vert.spv";
    shader_settings.frag_addr = "../Shaders/frag.spv";
    shader_settings.binding_description = Vertex::getBindingDescription();
    const auto& attribute_descriptions = Vertex::getAttributeDescriptions();
    shader_settings.input_attribute_descriptions = attribute_descriptions.data();
    shader_settings.input_attribute_descriptions_count = attribute_descriptions.size();

    {
        auto& layout = shader_settings.descriptor_set_layouts.emplace_back();
        layout.update_per_frame = true;

        // Proj view model mat
        {
            auto& binding = layout.bindings.emplace_back();
            binding.stage_flags = VK_SHADER_STAGE_VERTEX_BIT;
            binding.uniform_data_size = sizeof(UniformData);
            binding.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    
        }

        // Texture sampler
        {
            auto& binding = layout.bindings.emplace_back();
            binding.stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT;
            binding.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
    }
    
    loadModel("../Models/viking_room_gltf/scene.gltf", glm::mat4(1.0f), meshes, instance.device_manager);

    glm::mat4 duck_mat = glm::mat4(1.0f);
    duck_mat = glm::translate(duck_mat, glm::vec3(0.0f, 10.0f, 0.0f));
    duck_mat = glm::scale(duck_mat, glm::vec3(0.02f));
    loadModel("../Models/duck_gltf/Duck.gltf", duck_mat, meshes, instance.device_manager);

    std::vector<Texture*> textures;
    for (auto& mesh : meshes)
        textures.push_back(&mesh.texture);

    shader_settings.uniform_descriptor_count = textures.size();
    shader_settings.texture_descriptor_count = textures.size();
    shader_settings.descriptor_set_count = textures.size();

    // Create descriptor pool
    instance.descriptor_pool.init(instance.device_manager.logicalDevice, instance.swapchain.images.size() * shader_settings.uniform_descriptor_count, instance.swapchain.images.size() * shader_settings.texture_descriptor_count, instance.swapchain.images.size() * shader_settings.descriptor_set_count);

    for (auto& layout : shader_settings.descriptor_set_layouts)
        layout.upload(instance.device_manager);

    // Create uniform buffers
    {
        VkDeviceSize uniform_data_size = 0;
        for (const auto& binding : shader_settings.descriptor_set_layouts[0].bindings)
        {
            if (binding.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                uniform_data_size += binding.uniform_data_size;
        }

        uniform_buffers.resize(instance.swapchain.images.size());
        for (auto& buffer : uniform_buffers)
        {
            buffer.init(instance.device_manager.physicalDevice, instance.device_manager.logicalDevice, uniform_data_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }
    
    instance.pipeline.init(instance.device_manager, instance.swapchain, shader_settings);

    descriptor_sets.resize(instance.pipeline.swapchain_image_size * textures.size());
    std::vector<Buffer*> tmp_uniform_buffers(1);
    std::vector<Texture*> tmp_textures(1);
    for (size_t i = 0; i < textures.size(); ++i)
    {
        for (size_t j = 0; j < instance.pipeline.swapchain_image_size; ++j)
        {
            tmp_uniform_buffers[0] = &uniform_buffers[j];
            tmp_textures[0] = textures[i];
            descriptor_sets[i * instance.pipeline.swapchain_image_size + j] = instance.descriptor_pool.createDescriptorSet(instance.device_manager.logicalDevice, shader_settings.descriptor_set_layouts[0], tmp_uniform_buffers, tmp_textures);
        }
    }
    
    instance.mainLoop();

    for (auto& mesh : meshes)
    {
        mesh.vertex_buffer.deinit(instance.device_manager.logicalDevice);
        mesh.index_buffer.deinit(instance.device_manager.logicalDevice);
        mesh.texture.deinit(instance.device_manager.logicalDevice);
    }

    for (auto& buffer : uniform_buffers)
        buffer.deinit(instance.device_manager.logicalDevice);

    for (auto& layout : shader_settings.descriptor_set_layouts)
        layout.deinit(instance.device_manager);

    instance.deinit();
}