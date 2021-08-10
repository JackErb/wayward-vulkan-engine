#version 450

#define MAX_OBJECTS 32

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTextureIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint fragTextureIndex;
layout(location = 2) out vec4 fragPosition;

void main() {
    vec3 position = inPosition;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);

    fragTexCoord = inTexCoord;
    fragTextureIndex = inTextureIndex;
    fragPosition = gl_Position;
}
