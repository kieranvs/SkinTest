#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    //outColour = vec4(fragColour * texture(texSampler, fragTexCoord).rgb, 1.0f);
    outColour = vec4(texture(texSampler, fragTexCoord).rgb, 1.0f);
}