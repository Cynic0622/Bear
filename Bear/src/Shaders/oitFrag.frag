#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;
#include "..\Renderer\BaseData.h"
#include "common.glsli"
#define MAX_IBL_LOD 4.0
struct Node
{
    vec4 color;
    float depth;
    uint next;
};

layout(set = 1, binding = 0) uniform sceneDataBlock {
    SceneData g_SceneData;
};
layout(set = 1, binding = 1) uniform sampler2D IBLSampler;
layout(set = 2, binding = 0) uniform Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3 emissiveFactor;
} materialData;
layout(set = 2, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 2, binding = 2) uniform sampler2D normalSampler;
layout(set = 2, binding = 3) uniform sampler2D metallicRoughnessSampler;
layout(set = 2, binding = 4) uniform sampler2D occlusionSampler;
layout(set = 2, binding = 5) uniform sampler2D emissiveSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 viewDir;

layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout (set = 3, binding = 0) buffer LinkedListSBO
{
    Node nodes[];
};
layout (set = 3, binding = 1) buffer AtomicCounter
{
    uint counter;
    uint maxNodeCount;
};

layout (set = 3, binding = 2, r32ui) uniform uimage2D headIndexImage;

void main() {
    mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);

    vec3 albedo = baseColor.rgb * materialData.baseColorFactor.rgb;
    float alpha = baseColor.a * materialData.baseColorFactor.a;
    if (alpha < 0.001) discard;

    vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
    normal.xy *= materialData.normalScale;
    normal = normalize(normal);
    vec3 N = normalize(TBN * normal);

    vec3 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rgb;
    float metallic = clamp(materialData.metallicFactor * metallicRoughness.b, 0.0, 1.0);
    float roughness = clamp(materialData.roughnessFactor * metallicRoughness.g, 0.04, 1.0);

    float ao = texture(occlusionSampler, fragTexCoord).r * materialData.occlusionStrength;
    vec3 emissive = texture(emissiveSampler, fragTexCoord).rgb * materialData.emissiveFactor;

    vec3 V = normalize(viewDir);
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    // point lights - Cook-Torrance BRDF
    for (int i = 0; i < g_SceneData.numLights; ++i)
    {
        vec3 lightPos = g_SceneData.pointLights[i].position.xyz;
        vec3 L = normalize(lightPos - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = g_SceneData.pointLights[i].color.rgb * g_SceneData.pointLights[i].color.a * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, (roughness + 1.0) * (roughness + 1.0) / 8.0);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denom;

        float NdotL = max(dot(N, L), 0.0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 diffuse = kD * albedo / 3.14159265;

        Lo += (diffuse + specular) * radiance * NdotL;
    }
    // directional light
    vec3 L = normalize(-g_SceneData.dirLight.direction.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance = g_SceneData.dirLight.color.rgb * g_SceneData.dirLight.color.a;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, (roughness + 1.0) * (roughness + 1.0) / 8.0);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denom;

    float NdotL = max(dot(N, L), 0.0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = kD * albedo / 3.14159265;

    Lo += (diffuse + specular) * radiance * NdotL;

    // IBL
    float lod = roughness * MAX_IBL_LOD;
    vec3 iblReflection = textureLod(IBLSampler, vec2(
        atan(R.z, R.x) / (2.0 * 3.14159265) + 0.5,
        asin(R.y) / 3.14159265 + 0.5), lod).rgb;
    iblReflection = iblReflection * exp2(-2.0);
    Lo += iblReflection * F;

    vec3 ambient = vec3(0.15) * albedo * ao;
    vec3 color = ambient + Lo + emissive;
    color = ACESFilm(color);

    uint nodeIdx = atomicAdd(counter, 1);

    if (nodeIdx < maxNodeCount)
    {
        uint prevHeadIdx = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIdx);
        nodes[nodeIdx].color = vec4(color, alpha);
        nodes[nodeIdx].next = prevHeadIdx;
        nodes[nodeIdx].depth = gl_FragCoord.z;
    }
}
