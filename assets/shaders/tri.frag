#version 450

layout(set = 1, binding = 0) uniform Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3 emissiveFactor;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 4) uniform sampler2D occlusionSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveSampler;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);
    if (baseColor.a < 0.1) discard;
    vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
    vec2 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).xy;
    float occlusion = texture(occlusionSampler, fragTexCoord).r;
    vec3 emissive = texture(emissiveSampler, fragTexCoord).rgb;

    // Combine the textures and material properties
    outColor = baseColor * materialData.baseColorFactor + vec4(emissive * materialData.emissiveFactor, 0.0);
}