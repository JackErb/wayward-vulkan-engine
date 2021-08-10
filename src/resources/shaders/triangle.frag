#version 450

#define MAX_TEXTURES 2

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) flat in uint textureIndex;
layout(location = 2) in vec4 inPosition;

layout(binding = 1) uniform texture2D textures[MAX_TEXTURES];
layout(binding = 2) uniform sampler texSampler;
layout(binding = 3) uniform sampler2D depthSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(textures[textureIndex], texSampler), inTexCoord);

    /*vec2 depthPosition = vec2(inPosition.x / inPosition.w, inPosition.y / inPosition.w);

    vec4 depthColor = texture(depthSampler, depthPosition);
    float depth = depthColor.r;

    outColor = vec4(depth, depth, depth, 1.f);*/
}
