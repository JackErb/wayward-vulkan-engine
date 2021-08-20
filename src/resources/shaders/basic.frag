#version 450

#define MAX_TEXTURES 2

layout(location = 0) in vec2 texCoord;
layout(location = 1) flat in uint textureIndex;
layout(location = 2) in vec4 lightPosition;
layout(location = 3) in vec3 vertNormal;
layout(location = 4) in vec3 worldPosition;

layout(binding = 1) uniform texture2D textures[MAX_TEXTURES];
layout(binding = 2) uniform sampler texSampler;
layout(binding = 3) uniform sampler2D depthSampler;

layout(location = 0) out vec4 outColor;

const vec3 lightPos = vec3(1.0, 1.0, 1.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 diffuseColor = vec3(1.0, 1.0, 1.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

void main() {
    /* === SHADOW CALCULATIONS === */

    float shadow = 0.0;

    vec3 projCoords = lightPosition.xyz / lightPosition.w;
    vec2 depthTexCoords = projCoords.xy * 0.5 + 0.5;
    if ( (depthTexCoords.x >= 0.0 && depthTexCoords.x <= 1.0) && (depthTexCoords.y >= 0.0 && depthTexCoords.y <= 1.0) )  {
        vec4 depthColor = texture(depthSampler, depthTexCoords);
        float lightDepth = depthColor.r;
        float fragDepth = projCoords.z;

        if (fragDepth - 0.01 > lightDepth) {
            // We are in shadow
            shadow = 1.0;
        }
    } else {
        // Tex coordinates are outside outside of depth map. Assume we are not in shadow
        shadow = 0.0;
    }

    /* === LIGHTING CALCULATIONS === */

    vec4 textureColor = texture(sampler2D(textures[textureIndex], texSampler), texCoord);

    vec3 normal = normalize(vertNormal);
    vec3 lightDir = normalize(lightPos - worldPosition);
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    vec3 ambient = textureColor.rgb * 0.15;

    float lambertian = max(dot(lightDir, normal), 0.0);

    vec3 diffuse = lambertian * lightColor;

    vec3 specular = vec3(0.0);
    if (lambertian > 0.0) {
        vec3 viewDir = normalize(-worldPosition);
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, shininess) * lightColor;
    }

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * textureColor.rgb;
    outColor = vec4(lighting, textureColor.a);


    /*
    // Set texture to depth sampler
    vec4 depthTexture = texture(depthSampler, texCoord);
    float depth = depthTexture.r;
    outColor = vec4(vec3(depth), 1.0);
    */
}
