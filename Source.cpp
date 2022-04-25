#include "VulkanInstance.h"
#include "Vertex.h"

int main()
{
    VulkanWrapper::Buffer vertex_buffer;
    VulkanWrapper::Buffer index_buffer;

    VulkanInstance instance;
    instance.shader_settings.vert_addr = "../Shaders/vert.spv";
    instance.shader_settings.frag_addr = "../Shaders/frag.spv";
    instance.shader_settings.binding_description = Vertex::getBindingDescription();
    const auto& attribute_descriptions = Vertex::getAttributeDescriptions();
    instance.shader_settings.input_attribute_descriptions = attribute_descriptions.data();
    instance.shader_settings.input_attribute_descriptions_count = attribute_descriptions.size();
    instance.shader_settings.uniform_data_size = sizeof(UniformData);

    instance.command_buffer_callback = [&vertex_buffer, &index_buffer](const VulkanWrapper::Pipeline& pipeline, const size_t i, const VkCommandBuffer command_buffer) {
        VkBuffer vertexBuffers[] = { vertex_buffer.handle };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(command_buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &pipeline.descriptor_sets[i], 0, nullptr);

        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(index_buffer.count), 1, 0, 0, 0);
    };

    instance.init();

    std::vector<Vertex> verts{};
    std::vector<uint32_t> indices{0, 1, 2};
    verts.resize(3);
    verts[0].pos = { -0.5, -0.5, 0.0 };
    verts[1].pos = { 0.5, -0.5, 0.0 };
    verts[2].pos = { 0.0, 0.5, 0.0 };
    verts[0].colour = { 1.0, 0.0, 0.0 };
    verts[1].colour = { 0.0, 1.0, 0.0 };
    verts[2].colour = { 0.0, 0.0, 1.0 };

    uploadBufferData(instance.device_manager, vertex_buffer, verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    uploadBufferData(instance.device_manager, index_buffer, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    instance.mainLoop();

    vertex_buffer.deinit(instance.device_manager.logicalDevice);
    index_buffer.deinit(instance.device_manager.logicalDevice);
    instance.deinit();
}