#version 450

layout(binding = 0) uniform CameraTransform {
    mat4 View;
    mat4 Proj;
} Camera;

layout(binding = 4) uniform LightTransform {
    mat4 View;
    mat4 Proj;
} Light;

/* TODO : Read the size of this object using spirv-reflect during runtime */
#define MAX_OBJECTS  8
#define MAX_JOINTS  16
layout(binding = 5) uniform ObjectData {
    mat4 Models[MAX_OBJECTS];
    mat4 Joints[MAX_OBJECTS * MAX_JOINTS];
} Objects;

//push constants block
layout(push_constant) uniform constants
{
    uint ObjectId;
} PushConstants;

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoord;
layout(location = 3) in uint TextureIndex;
layout(location = 4) in uint Joint1;
layout(location = 5) in uint Joint2;
layout(location = 6) in float Weight1;
layout(location = 7) in float Weight2;

layout(location = 0) out vec2 FragTexCoord;
layout(location = 1) flat out uint FragTextureIndex;
layout(location = 2) out vec4 FragLightPosition;
layout(location = 3) out vec3 FragNormal;
layout(location = 4) out vec3 FragWorldPosition;

mat4 getModel() {
    return Objects.Models[PushConstants.ObjectId];
}

mat4 getJoint(uint jointId) {
    return Objects.Joints[PushConstants.ObjectId * MAX_JOINTS + jointId];
}

void main() {
    vec4 position = getModel() * vec4(VertexPosition, 1.0);
    vec4 riggedPosition = Weight1 * getJoint(Joint1) * position
                        + Weight2 * getJoint(Joint2) * position;

    gl_Position = Camera.Proj * Camera.View * riggedPosition;

    FragTexCoord = TexCoord;
    FragTextureIndex = TextureIndex;
    FragNormal = Normal;
    FragLightPosition = Light.Proj * Light.View * riggedPosition;
    FragWorldPosition = riggedPosition.xyz / riggedPosition.w;
}
