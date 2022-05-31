#pragma once

#include "Vertex.h"
#include "VulkanWrapper/Buffer.h"
#include "VulkanWrapper/DeviceManager.h"
#include "VulkanWrapper/Image.h"

#include <string>
#include <vector>

using namespace VulkanWrapper;

struct Mesh
{
    VulkanWrapper::Buffer vertex_buffer;
    VulkanWrapper::Buffer index_buffer;
    Texture texture;
};

void loadModel(const std::string& file_path, const glm::mat4& bake_transform, std::vector<Mesh>& meshes, DeviceManager& device_manager);