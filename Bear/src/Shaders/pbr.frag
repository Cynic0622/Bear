#version 450

#include "..\Renderer\BaseData.h"
#define MAX_IBL_LOD 4.0
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
layout(location = 0) out vec4 outColor;

#include "common.glsli"

void main() {
    mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);

    vec3 albedo = baseColor.rgb * materialData.baseColorFactor.rgb;
    float alpha = baseColor.a * materialData.baseColorFactor.a;
    if (alpha < 0.5) discard;

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
    // point light
    for (int i = 0; i < g_SceneData.numLights; ++i)
    {
        vec3 lightPos = g_SceneData.pointLights[i].position.xyz;
        vec3 L = normalize(lightPos - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (distance * distance); // simple quadratic attenuation
        // attenuation = 1.0 / distance; // linear attenuation
        vec3 radiance = g_SceneData.pointLights[i].color.rgb * g_SceneData.pointLights[i].color.a * attenuation;

        // cook-torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, (roughness+1.0)*(roughness+1.0)/8.0);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denom;

        float NdotL = max(dot(N, L), 0.0);

        // kS is specular, kD is diffuse (energy conservation)
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        // Lambert diffuse
        vec3 diffuse = kD * albedo / 3.14159265;

        Lo += (diffuse + specular) * radiance * NdotL;
    }
    // directional light
    vec3 L = normalize(-g_SceneData.dirLight.direction.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance = g_SceneData.dirLight.color.rgb * g_SceneData.dirLight.color.a;
    // cook-torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, (roughness+1.0)*(roughness+1.0)/8.0);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denom;
    float NdotL = max(dot(N, L), 0.0);
    // kS is specular, kD is diffuse (energy conservation)
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    // Lambert diffuse
    vec3 diffuse = kD * albedo / PI;
    Lo += (diffuse + specular) * radiance * NdotL;

    // IBL (split-sum approximation)
    float NdotV = max(dot(N, V), 0.0);

    // Diffuse IBL — sample with normal at max blur
    vec3 irradiance = textureLod(IBLSampler, vec2(
        atan(N.z, N.x) / (2.0 * PI) + 0.5,
        asin(N.y) / PI + 0.5), MAX_IBL_LOD).rgb * exp2(-2.0);
    vec3 F_ibl = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD_ibl = (vec3(1.0) - F_ibl) * (1.0 - metallic);
    vec3 diffuseIBL = kD_ibl * albedo * irradiance;

    // Specular IBL — sample with reflection at roughness-dependent LOD
    float lod = roughness * MAX_IBL_LOD;
    vec3 prefiltered = textureLod(IBLSampler, vec2(
        atan(R.z, R.x) / (2.0 * PI) + 0.5,
        asin(R.y) / PI + 0.5), lod).rgb * exp2(-2.0);
    vec3 specularIBL = prefiltered * EnvBRDFApprox(F0, roughness, NdotV);

    Lo += diffuseIBL + specularIBL;

    vec3 ambient = vec3(0.15) * albedo * ao;
    vec3 color = ambient + Lo + emissive;
    color = ACESFilm(color);
    outColor = vec4(color, alpha);
}