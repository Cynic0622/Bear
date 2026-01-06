#version 450
#include "..\Renderer\BaseData.h"
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

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 ACESFilm(vec3 x)
{
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0
    );
}

void main() {
    mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);

    vec3 albedo = baseColor.rgb * materialData.baseColorFactor.rgb;
    float alpha = baseColor.a * materialData.baseColorFactor.a;
    if (baseColor.a < 0.2) discard;

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

    // pbr render cook-torrance
    for (int i = 0; i < g_SceneData.numLights; ++i)
    {
        vec3 lightPos = g_SceneData.pointLights[i].position.xyz;
        vec3 L = normalize(lightPos - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (distance * distance); // simple quadratic attenuation
        vec3 radiance = g_SceneData.pointLights[i].color.rgb * g_SceneData.pointLights[i].color.a * attenuation;

        // blin-phong
        float shininess = clamp(2.0 / (roughness * roughness) - 2.0, 1.0, 4096.0);
        vec3 spec = pow(max(dot(N, H), 0.0), shininess) * radiance;
        vec3 diff = max(dot(N, L), 0.0) * radiance;

        // kS is specular, kD is diffuse (energy conservation)
        vec3 kS = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        // Lambert diffuse
        vec3 diffuse = kD * diff;
        vec3 specular = kS * spec;
        Lo += diffuse + specular;
    }
    // directional light
    vec3 L = normalize(-g_SceneData.dirLight.direction.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance = g_SceneData.dirLight.color.rgb * g_SceneData.dirLight.color.a;
        
    float shininess = clamp(2.0 / (roughness * roughness) - 2.0, 1.0, 4096.0);
    vec3 spec = pow(max(dot(N, H), 0.0), shininess) * radiance;
    vec3 diff = max(dot(N, L), 0.0) * radiance;

    vec3 kS = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = kD * diff;
    vec3 specular = kS * spec;

    Lo += diffuse + specular;

    float lod = roughness * MAX_IBL_LOD;
    vec3 iblReflection = texture(IBLSampler, vec2(
		atan(R.z, R.x) / (2.0 * 3.14159265) + 0.5,
		asin(R.y) / 3.14159265 + 0.5)).rgb;
    iblReflection = iblReflection * exp2(-2.0);
    vec3 L1 = iblReflection * kS;
    Lo += ACESFilm(L1);

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo + emissive;
    // color = color / (color + vec3(1.0));
    // vec4 color = baseColor * materialData.baseColorFactor;
    uint nodeIdx = atomicAdd(counter, 1);

    if (nodeIdx < maxNodeCount)
    {
        uint prevHeadIdx = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIdx);
        // Write the fragment data to the linked list
        nodes[nodeIdx].color = vec4(color, alpha);
        // nodes[nodeIdx].color = color;
        nodes[nodeIdx].next = prevHeadIdx;
        nodes[nodeIdx].depth = gl_FragCoord.z;
    }
}