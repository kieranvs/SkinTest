#include "VulkanInstance.h"

int main()
{
    VulkanInstance instance;
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

    instance.createBuffers(verts, indices);

    instance.deinit();
}