#pragma once

#include "Vertex.h"

#include <string>
#include <vector>

std::pair<std::vector<Vertex>, std::string> loadModel(const std::string& file_path, const glm::mat4& bake_transform);