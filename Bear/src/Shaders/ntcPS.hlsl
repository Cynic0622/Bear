// Neural texture compression material shading.
//
// The lighting here mirrors pbr.frag (same BRDF, IBL, cascaded PCSS shadows, ambient term and
// ACES tone mapping) so that toggling texture compression does not change how the scene looks.
// The helpers are HLSL copies of common.glsli / pbr.frag - keep both sides in sync when either
// changes. Material factors (baseColorFactor, metallicFactor, roughnessFactor, normalScale,
// occlusionStrength, emissiveFactor) are not applied yet: NTC bakes constants for missing maps
// only, and every material in this repository uses the default (identity) factors.

#include "libntc/shaders/InferenceConstants.h"
#include "libntc/shaders/Inference.hlsli"
#include "NtcChannelMapping.h"
#include "BaseData.h"
#include "STFSamplerState.hlsli"
#include "shaders/HashBasedRNG.hlsli"

#define MAX_IBL_LOD 4.0
#define PI 3.14159265

static const float kGoldenAngle = 2.39996323;
static const float kMaxFilterTexels = 16.0; // upper clamp of the PCSS filter radius, in shadow map texels

struct MaterialTextureSample
{
    float4 baseOrDiffuse;
    float4 metalRoughOrSpecular;
    float4 normal;
    float4 emissive;
    float4 occlusion;
    float4 transmission;
    float opacity;
};

// base data
[[vk::binding(0, 0)]] ConstantBuffer<BaseData> g_BaseData;
[[vk::binding(0, 1)]] ConstantBuffer<SceneData> g_SceneData;

// Combined image samplers: the texture and its sampler share one binding, which is how dxc
// emits a single VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER descriptor.
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] Texture2D IBLSampler;
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] SamplerState IBLSamplerState;
[[vk::combinedImageSampler]] [[vk::binding(3, 1)]] Texture2DArray shadowMap;
[[vk::combinedImageSampler]] [[vk::binding(3, 1)]] SamplerState shadowMapSampler;

[[vk::binding(0, 2)]] ByteAddressBuffer t_InputFile;    // the latent data.
[[vk::binding(1, 2)]] ByteAddressBuffer t_WeightBuffer; // the neural network weights.
[[vk::binding(2, 2)]] ConstantBuffer<NtcTextureSetConstants> g_NtcMaterial;

// ---------------------------------------------------------------------------------------------
// BRDF and tone mapping, mirroring common.glsli
// ---------------------------------------------------------------------------------------------

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0)
        * pow(1.0 - cosTheta, 5.0);
}

float3 EnvBRDFApprox(float3 F0, float roughness, float NdotV)
{
    float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
    float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
    return F0 * AB.x + AB.y;
}

float3 ACESFilm(float3 x)
{
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0);
}

// ---------------------------------------------------------------------------------------------
// Cascaded PCSS shadows, mirroring pbr.frag
// ---------------------------------------------------------------------------------------------

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

// percentage-closer soft shadows for one explicit cascade: blocker search -> variable-radius PCF
float computeShadowFromCascade(int cascade, float3 worldPos, float3 N)
{
    float texelWorld = g_SceneData.cascadeTexelWorld[cascade];
    float depthRange = max(g_SceneData.cascadeDepthRange[cascade], 1e-4);

    // normal offset reduces acne without a large depth bias
    float3 samplePos = worldPos + N * (texelWorld * g_SceneData.shadowParams.z);

    float4 lightClip = mul(g_SceneData.lightViewProj[cascade], float4(samplePos, 1.0));
    float3 proj = lightClip.xyz / lightClip.w;
    proj.xy = proj.xy * 0.5 + 0.5;
    // bias is authored in world units, the ortho depth is normalized over the cascade's depth range
    proj.z -= g_SceneData.shadowParams.x / depthRange;
    if (proj.z <= 0.0 || proj.z >= 1.0)
        return 1.0;

    uint mapWidth, mapHeight, mapLayers;
    shadowMap.GetDimensions(mapWidth, mapHeight, mapLayers);
    float2 mapSize = float2(mapWidth, mapHeight);
    float searchRadius = 4.0 / mapSize.x;
    float lightSize = g_SceneData.shadowParams.y;

    // blocker search
    float blockerSum = 0.0;
    float blockerCount = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float angle = float(i) * kGoldenAngle;
        float radius = sqrt((float(i) + 0.5) / 16.0) * searchRadius;
        float2 offset = float2(cos(angle), sin(angle)) * radius;
        float d = shadowMap.Sample(shadowMapSampler, float3(proj.xy + offset, float(cascade))).r;
        if (d < proj.z)
        {
            blockerSum += d;
            blockerCount += 1.0;
        }
    }
    if (blockerCount < 0.5)
        return 1.0;

    float avgBlocker = blockerSum / blockerCount;

    // Directional light: penumbra grows with the world-space receiver<->blocker gap measured
    // along the light axis. proj.z/avgBlocker are ortho depths normalized to [0,1] across the
    // cascade box, so scaling their difference by the box size recovers the world gap.
    float boxSizeWorld = texelWorld * mapSize.x;            // == 2.0 * cascade radius
    float gapWorld = max((proj.z - avgBlocker) * boxSizeWorld, 0.0);
    float penumbraWorld = gapWorld * lightSize;             // lightSize: sun angular factor
    float filterRadius = clamp(penumbraWorld / texelWorld, 1.0, kMaxFilterTexels) / mapSize.x;

    float shadow = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        float angle = float(i) * kGoldenAngle;
        float radius = sqrt((float(i) + 0.5) / 16.0) * filterRadius;
        float2 offset = float2(cos(angle), sin(angle)) * radius;
        float d = shadowMap.Sample(shadowMapSampler, float3(proj.xy + offset, float(cascade))).r;
        shadow += (d >= proj.z) ? 1.0 : 0.0;
    }
    return shadow / 16.0;
}

// percentage-closer soft shadows: pick the cascade for this fragment, then filter
float computeShadow(float3 worldPos, float3 N)
{
    float viewDepth = -mul(g_BaseData.viewMat, float4(worldPos, 1.0)).z;
    int cascadeCount = int(g_SceneData.shadowParams.w);
    if (viewDepth > g_SceneData.cascadeSplits[cascadeCount - 1])
        return 1.0;
    return computeShadowFromCascade(selectCascade(viewDepth), worldPos, N);
}

// ---------------------------------------------------------------------------------------------
// NTC sampling (stochastic filtering + per-pixel inference)
// ---------------------------------------------------------------------------------------------

void GetSamplePositionWithSTF(inout HashBasedRNG rng, float2 uv, out int2 texel, out int mipLevel)
{
    float4 random = rng.NextFloat4();
    STF_SamplerState sampler = STF_SamplerState::Create(random);
    sampler.SetAnisoMethod(STF_ANISO_LOD_METHOD_DEFAULT);
    sampler.SetFilterType(STF_FILTER_TYPE_GAUSSIAN);

    const int2 textureSize = NtcGetTextureDimensions(g_NtcMaterial, 0);
    const int mipLevels = NtcGetTextureMipLevels(g_NtcMaterial);
    float3 samplePos = sampler.Texture2DGetSamplePos(textureSize.x, textureSize.y, mipLevels, uv);
    mipLevel = int(samplePos.z);

    const int2 mipSize = NtcGetTextureDimensions(g_NtcMaterial, mipLevel);

    bool border;
    samplePos.xy = STF_ApplyAddressingMode2D(samplePos.xy, mipSize, STF_ADDRESS_MODE_WRAP, border);

    texel = int2(floor(samplePos.xy * mipSize));
}

MaterialTextureSample SampleNtcMaterial(uint2 pixelPosition, float2 uv)
{
    HashBasedRNG rng = HashBasedRNG::Create2D(pixelPosition, g_BaseData.frameIndex);
    int2 texel;
    int mipLevel;
    GetSamplePositionWithSTF(rng, uv, texel, mipLevel);
    // The NtcSampleTextureSet... functions can convert all channels to linear color based on metadata stored
    // in the constant buffer. But that can be relatively slow if not optimized away by the driver.
    // Since we know the color spaces for all channels in advance, linearize explicitly below.
    const bool linearizeColorsOnSample = false;

    // Decompress the texel and get all the channels.
    float channels[NtcNetworkParams<NTC_NETWORK_LARGE>::OUTPUT_CHANNELS];
    NtcSampleTextureSet<NTC_NETWORK_LARGE>(g_NtcMaterial, t_InputFile, 0,
        t_WeightBuffer, 0, texel, mipLevel, linearizeColorsOnSample, channels);

    MaterialTextureSample textures;

    textures.baseOrDiffuse.rgb = float3(
        channels[CHANNEL_BASE_COLOR + 0],
        channels[CHANNEL_BASE_COLOR + 1],
        channels[CHANNEL_BASE_COLOR + 2]);

    // pbr.frag samples an sRGB image view, where the hardware decodes to linear on sample.
    if (!linearizeColorsOnSample)
        textures.baseOrDiffuse.rgb = NtcSrgbColorSpace::Decode(textures.baseOrDiffuse.rgb);

    textures.opacity.r = channels[CHANNEL_OPACITY];
    textures.metalRoughOrSpecular.g = channels[CHANNEL_ROUGHNESS];
    textures.metalRoughOrSpecular.r = channels[CHANNEL_METALNESS];

    textures.normal.rgb = float3(
        channels[CHANNEL_NORMAL + 0],
        channels[CHANNEL_NORMAL + 1],
        channels[CHANNEL_NORMAL + 2]);

    textures.occlusion.r = channels[CHANNEL_OCCLUSION];

    textures.emissive.rgb = float3(
        channels[CHANNEL_EMISSIVE + 0],
        channels[CHANNEL_EMISSIVE + 1],
        channels[CHANNEL_EMISSIVE + 2]);

    if (!linearizeColorsOnSample)
        textures.emissive.rgb = NtcSrgbColorSpace::Decode(textures.emissive.rgb);

    textures.transmission.r = channels[CHANNEL_TRANSMISSION];

    return textures;
}

#define VK_LOCATION_INDEX(loc, idx) [[vk::location(loc)]] [[vk::index(idx)]]

void main(
    in float4 i_position : SV_Position,   // the pixel position in screen space.
    in float2 i_uv : TEXCOORD,            // the interpolated vertex data for this pixel.
    in float4 i_worldPos : TEXCOORD1,     // world position of the pixel
    in float3x3 i_tbn : TEXCOORD2,        // tangent, bitangent, normal matrix for the pixel
    VK_LOCATION_INDEX(0, 0) out float4 o_color : SV_Target0)
{
    MaterialTextureSample textures = SampleNtcMaterial(uint2(i_position.xy), i_uv);

    float3 albedo = textures.baseOrDiffuse.rgb;
    float alpha = textures.opacity;
    if (alpha < 0.5)
        discard;

    float3 normal = normalize(textures.normal.rgb * 2.0 - 1.0);
    float3 N = normalize(mul(normal, i_tbn));
    float metallic = clamp(textures.metalRoughOrSpecular.r, 0.0, 1.0);
    float roughness = clamp(textures.metalRoughOrSpecular.g, 0.04, 1.0);
    float ao = textures.occlusion.r;
    float3 emissive = textures.emissive.rgb;

    float3 fragPos = i_worldPos.xyz;
    float3 V = normalize(g_BaseData.cameraPosition.xyz - fragPos);
    float3 R = reflect(-V, N);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // point lights
    for (int i = 0; i < g_SceneData.numLights; ++i)
    {
        float3 lightPos = g_SceneData.pointLights[i].position.xyz;
        float3 L = normalize(lightPos - fragPos);
        float3 H = normalize(V + L);
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = g_SceneData.pointLights[i].color.rgb * g_SceneData.pointLights[i].color.a * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, (roughness + 1.0) * (roughness + 1.0) / 8.0);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        float3 specular = numerator / denom;
        float NdotL = max(dot(N, L), 0.0);

        float3 kS = F;
        float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
        float3 diffuse = kD * albedo / PI;

        Lo += (diffuse + specular) * radiance * NdotL;
    }

    // directional light
    float dirShadow = 1.0;
    {
        float3 L = normalize(-g_SceneData.dirLight.direction.xyz);
        float3 H = normalize(V + L);
        float3 radiance = g_SceneData.dirLight.color.rgb * g_SceneData.dirLight.color.a;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, (roughness + 1.0) * (roughness + 1.0) / 8.0);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        float3 specular = numerator / denom;
        float NdotL = max(dot(N, L), 0.0);

        float3 kS = F;
        float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
        float3 diffuse = kD * albedo / PI;

        dirShadow = computeShadow(fragPos, N);
        Lo += (diffuse + specular) * radiance * NdotL * dirShadow;
    }

    // IBL (split-sum approximation)
    float NdotV = max(dot(N, V), 0.0);

    // Diffuse IBL - sample with normal at max blur
    float3 irradiance = IBLSampler.SampleLevel(IBLSamplerState, float2(
        atan2(N.z, N.x) / (2.0 * PI) + 0.5,
        asin(N.y) / PI + 0.5), MAX_IBL_LOD).rgb * exp2(-2.0);
    float3 F_ibl = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kD_ibl = (float3(1.0, 1.0, 1.0) - F_ibl) * (1.0 - metallic);
    float3 diffuseIBL = kD_ibl * albedo * irradiance;

    // Specular IBL - sample with reflection at roughness-dependent LOD
    float lod = roughness * MAX_IBL_LOD;
    float3 prefiltered = IBLSampler.SampleLevel(IBLSamplerState, float2(
        atan2(R.z, R.x) / (2.0 * PI) + 0.5,
        asin(R.y) / PI + 0.5), lod).rgb * exp2(-2.0);
    float3 specularIBL = prefiltered * EnvBRDFApprox(F0, roughness, NdotV);

    Lo += diffuseIBL + specularIBL;

    float3 ambient = float3(0.15, 0.15, 0.15) * albedo * ao;
    float3 color = ambient + Lo + emissive;
    color = ACESFilm(color);

    // shadow debugging (same behaviour as pbr.frag for the H key)
    if (g_BaseData.shadowDebug == 1)
    {
        o_color = float4(float3(dirShadow, dirShadow, dirShadow), 1.0);
        return;
    }
    if (g_BaseData.shadowDebug == 2)
    {
        float viewDepth = -mul(g_BaseData.viewMat, float4(fragPos, 1.0)).z;
        int cascade = selectCascade(viewDepth);
        float3 cascadeColor = float3(cascade == 0 ? 1.0 : 0.1, cascade == 1 ? 1.0 : 0.1, cascade == 2 ? 1.0 : 0.1);
        if (cascade == 3)
            cascadeColor = float3(1.0, 1.0, 0.1);
        o_color = float4(cascadeColor, 1.0);
        return;
    }
    if (g_BaseData.shadowDebug == 3)
    {
        // raw shadow map depth along the current cascade projection (white = empty/cleared)
        float viewDepth = -mul(g_BaseData.viewMat, float4(fragPos, 1.0)).z;
        int cascade = selectCascade(viewDepth);
        float4 lightClip = mul(g_SceneData.lightViewProj[cascade], float4(fragPos, 1.0));
        float3 proj = lightClip.xyz / lightClip.w;
        proj.xy = proj.xy * 0.5 + 0.5;
        if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z < 0.0 || proj.z > 1.0)
        {
            o_color = float4(1.0, 0.0, 0.0, 1.0); // outside the cascade: red
            return;
        }
        float depth = shadowMap.Sample(shadowMapSampler, float3(proj.xy, float(cascade))).r;
        o_color = float4(float3(depth, depth, depth), 1.0);
        return;
    }

    o_color = float4(color, alpha);
}
