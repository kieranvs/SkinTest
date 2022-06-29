#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform ViewInfo {
    mat4 view;
    mat4 proj;
} view_info;
layout(set = 1, binding = 1) uniform ModelInfo {
    mat4 model;
} model_info;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = view_info.proj * view_info.view * model_info.model * vec4(inPosition, 1.0);
    fragColor = inColour;
    fragTexCoord = inTexCoord;
}