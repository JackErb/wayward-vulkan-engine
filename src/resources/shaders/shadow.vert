#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inTextureIndex;

void main() {
    vec3 position = inPosition;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
}
