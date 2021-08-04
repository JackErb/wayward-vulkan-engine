#version 450

#define MAX_TEXTURES 2

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) flat in uint textureIndex;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform texture2D textures[MAX_TEXTURES];
layout(binding = 1) uniform sampler texSampler;

void main() {
    outColor = texture(sampler2D(textures[textureIndex], texSampler), inTexCoord);
}
