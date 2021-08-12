#version 450

#define MAX_OBJECTS 32

layout(binding = 0) uniform CameraTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
} camera;

layout(binding = 4) uniform LightTransform {
    mat4 model;
    mat4 view;
    mat4 proj;
} light;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inTextureIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) flat out uint fragTextureIndex;
layout(location = 2) out vec4 lightPosition;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragWorldPosition;

void main() {
    vec3 position = inPosition;
    gl_Position = camera.proj * camera.view * camera.model * vec4(position, 1.0);

    fragTexCoord = inTexCoord;
    fragTextureIndex = inTextureIndex;
    fragNormal = inNormal;
    lightPosition = light.proj * light.view * light.model * vec4(position, 1.0);

    vec4 modelPosition = camera.model * vec4(position, 1.0);
    fragWorldPosition = vec3(modelPosition) / modelPosition.w;
}
