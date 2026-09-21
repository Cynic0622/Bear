#version 450

#include "..\Renderer\BaseData.h"
#define MAX_IBL_LOD 4.0
layout(set = 0, binding = 0) uniform baseDataBlock {
    BaseData g_BaseData;
};
layout(set = 1, binding = 0) uniform sceneDataBlock {
    SceneData g_SceneData;
};

layout(set = 1, binding = 1) uniform sampler2D IBLSampler;
layout(set = 1, binding = 3) uniform sampler2DArray shadowMap;

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

const float kGoldenAngle = 2.39996323;
const float kMaxFilterTexels = 16.0; // upper clamp of the PCSS filter radius, in shadow map texels

int selectCascade(float viewDepth)
{
    int count = int(g_SceneData.shadowParams.w);
    for (int c = 0; c < count - 1; ++c)
    {
        if (viewDepth < g_SceneData.cascadeSplits[c])
            return c;
    }
    return count - 1;
}

// percentage-closer soft shadows for one explicit cascade: blocker search -> variable-radius PCF.
// Kept separate from the cascade selection so callers can evaluate one explicit cascade.
float computeShadowFromCascade(int cascade, vec3 worldPos, vec3 N)
{
    float texelWorld = g_SceneData.cascadeTexelWorld[cascade];
    float depthRange = max(g_SceneData.cascadeDepthRange[cascade], 1e-4);

    // normal offset reduces acne without a large depth bias
    vec3 samplePos = worldPos + N * (texelWorld * g_SceneData.shadowParams.z);

    vec4 lightClip = g_SceneData.lightViewProj[cascade] * vec4(samplePos, 1.0);
    vec3 proj = lightClip.xyz / lightClip.w;
    proj.xy = proj.xy * 0.5 + 0.5;
    // bias is authored in world units, the ortho depth is normalized over the cascade's depth range
    proj.z -= g_SceneData.shadowParams.x / depthRange;
    if (proj.z <= 0.0 || proj.z >= 1.0)
        return 1.0;

    vec2 mapSize = vec2(textureSize(shadowMap, 0).xy);
    float searchRadius = 4.0 / mapSize.x;
    float lightSize = g_SceneData.shadowParams.y;

    // blocker search
    float blockerSum = 0.0;
    float blockerCount = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float angle = float(i) * kGoldenAngle;
        float radius = sqrt((float(i) + 0.5) / 16.0) * searchRadius;
        vec2 offset = vec2(cos(angle), sin(angle)) * radius;
        float d = texture(shadowMap, vec3(proj.xy + offset, cascade)).r;
        if (d < proj.z)
        {
            blockerSum += d;
            blockerCount += 1.0;
        }
    }
    if (blockerCount < 0.5)
        return 1.0;

    float avgBlocker = blockerSum / blockerCount;

    // Directional light: the light is at infinity, so the penumbra grows linearly with the
    // world-space receiver<->blocker gap measured along the light axis. proj.z/avgBlocker are
    // ortho depths normalized to [0,1] across the cascade box (2*radius), so scaling their
    // difference by the box size recovers the world gap. Dividing by avgBlocker instead (the
    // previous version) made the ratio depend on the fitted light "eye", which slides with the
    // camera: the filter radius then changed every frame and the soft shadow edges crawled
    // even though the shadow map's texel grid was world-anchored.
    float boxSizeWorld = texelWorld * mapSize.x;            // == 2.0 * cascade radius
    float gapWorld = max((proj.z - avgBlocker) * boxSizeWorld, 0.0);
    float penumbraWorld = gapWorld * lightSize;             // lightSize: sun angular factor
    float filterRadius = clamp(penumbraWorld / texelWorld, 1.0, kMaxFilterTexels) / mapSize.x;

    float shadow = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float angle = float(i) * kGoldenAngle;
        float radius = sqrt((float(i) + 0.5) / 16.0) * filterRadius;
        vec2 offset = vec2(cos(angle), sin(angle)) * radius;
        float d = texture(shadowMap, vec3(proj.xy + offset, cascade)).r;
        shadow += (d >= proj.z) ? 1.0 : 0.0;
    }
    return shadow / 16.0;
}

// percentage-closer soft shadows: pick the cascade for this fragment, then filter
float computeShadow(vec3 worldPos, vec3 N)
{
    float viewDepth = -(g_BaseData.viewMat * vec4(worldPos, 1.0)).z;
    int cascadeCount = int(g_SceneData.shadowParams.w);
    if (viewDepth > g_SceneData.cascadeSplits[cascadeCount - 1])
        return 1.0;
    return computeShadowFromCascade(selectCascade(viewDepth), worldPos, N);
}

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
    float dirShadow = computeShadow(fragPos, N);
    Lo += (diffuse + specular) * radiance * NdotL * dirShadow;

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

    // shadow debugging
    if (g_BaseData.shadowDebug == 1)
    {
        outColor = vec4(vec3(dirShadow), 1.0);
        return;
    }
    if (g_BaseData.shadowDebug == 2)
    {
        float viewDepth = -(g_BaseData.viewMat * vec4(fragPos, 1.0)).z;
        int cascade = selectCascade(viewDepth);
        vec3 cascadeColor = vec3(cascade == 0 ? 1.0 : 0.1, cascade == 1 ? 1.0 : 0.1, cascade == 2 ? 1.0 : 0.1);
        if (cascade == 3) cascadeColor = vec3(1.0, 1.0, 0.1);
        outColor = vec4(cascadeColor, 1.0);
        return;
    }
    if (g_BaseData.shadowDebug == 3)
    {
        // raw shadow map depth along the current cascade projection (white = empty/cleared)
        float viewDepth = -(g_BaseData.viewMat * vec4(fragPos, 1.0)).z;
        int cascade = selectCascade(viewDepth);
        vec4 lightClip = g_SceneData.lightViewProj[cascade] * vec4(fragPos, 1.0);
        vec3 proj = lightClip.xyz / lightClip.w;
        proj.xy = proj.xy * 0.5 + 0.5;
        if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z < 0.0 || proj.z > 1.0)
        {
            outColor = vec4(1.0, 0.0, 0.0, 1.0); // outside the cascade: red
            return;
        }
        float depth = texture(shadowMap, vec3(proj.xy, cascade)).r;
        outColor = vec4(vec3(depth), 1.0);
        return;
    }
    outColor = vec4(color, alpha);
}
