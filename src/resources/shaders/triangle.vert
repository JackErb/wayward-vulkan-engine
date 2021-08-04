#version 450

#define MAX_OBJECTS 32

layout(binding = 2) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

/*
layout(push_constant) uniform PER_OBJECT
{
    mat4 model;
    mat4 view;
    mat4 proj;
}pc;
*/

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTextureIndex;
//layout(location = 2) in int objectIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint fragTextureIndex;

/*layout(binding = 0) uniform ObjectData {
    glm::vec3 position;
    int textureIndex;
} objectData[MAX_OBJECTS];*/

void main() {
    vec3 position = inPosition;// + objectData[objectIndex].position;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);

    fragTexCoord = inTexCoord;
    fragTextureIndex = inTextureIndex;
}
