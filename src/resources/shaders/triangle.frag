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
const float lightPower = 50.0;
const vec3 ambientColor = vec3(0.1, 0.12, 0.1);
const vec3 diffuseColor = vec3(0.5, 0.0, 0.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 32.0;
const float screenGamma = 2.2;

void main() {
    vec4 textureColor = texture(sampler2D(textures[textureIndex], texSampler), texCoord);

    vec3 normal = normalize(vertNormal);
    vec3 lightDir = lightPos - worldPosition;
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {

        vec3 viewDir = normalize(-worldPosition);

        // this is blinn phong
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, shininess);
    }

    vec3 colorLinear = ambientColor +
                       diffuseColor * lambertian * lightColor * lightPower / distance +
                       specColor * specular * lightColor * lightPower / distance;

    // apply gamma correction (assume ambientColor, diffuseColor and specColor
    // have been linearized, i.e. have no gamma correction in them)
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));
    // use the gamma corrected color in the fragment
    outColor = textureColor * vec4(colorGammaCorrected, 1.0);

    vec2 depthPosition = vec2(lightPosition.x, lightPosition.y);
    vec4 depthColor = texture(depthSampler, depthPosition);
    float lightDepth = depthColor.r;
}
