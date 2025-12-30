// Include the constants header unconditionally so that NTC_NETWORK_UNKNOWN is always defined
#include "libntc/shaders/InferenceConstants.h"
#include "libntc/shaders/Inference.hlsli"
#include "NtcChannelMapping.h"
#include "BaseData.h"
#include "STFSamplerState.hlsli"
#include "shaders/HashBasedRNG.hlsli"

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

[[vk::binding(0, 2)]] ByteAddressBuffer t_InputFile; // the latent data.

[[vk::binding(1, 2)]] ByteAddressBuffer t_WeightBuffer;// the neural network weights.

[[vk::binding(2, 2)]] ConstantBuffer<NtcTextureSetConstants> g_NtcMaterial;

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

    // Initialize the 'textures' object with default values, just in case we miss something below.
    // MaterialTextureSample textures = DefaultMaterialTextures();
	MaterialTextureSample textures;
    // Distribute the NTC channels into the MaterialTextureSample's fields using a fixed mapping.
    // The mapping is enforced by the loader, see NtcMaterialLoader.cpp
    // If some texture channels are not present in the NTC material file, they are replaced with constant values
    // by the loader.

    textures.baseOrDiffuse.rgb = float3(
        channels[CHANNEL_BASE_COLOR + 0],
        channels[CHANNEL_BASE_COLOR + 1],
        channels[CHANNEL_BASE_COLOR + 2]);


    // if (!linearizeColorsOnSample)
    //     textures.baseOrDiffuse.rgb = NtcSrgbColorSpace::Decode(textures.baseOrDiffuse.rgb);

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
// #endif

#define VK_LOCATION_INDEX(loc, idx) [[vk::location(loc)]] [[vk::index(idx)]]
void main(
	in float4 i_position : SV_Position, // the pixel position in screen space.
	in float2 i_uv : TEXCOORD, // the interpolated vertex data for this pixel.
	in float4 i_worldPos : TEXCOORD1, // world position of the pixel
	in float3x3 i_tbn : TEXCOORD2, // tangent, bitangent, normal matrix for the pixel
	// in bool i_isFrontFace : SV_IsFrontFace, // whether the primitive is front-facing or back-facing.
	VK_LOCATION_INDEX(0, 0) out float4 o_color : SV_Target0 // the output color of this pixel.
)
{
    o_color = float4(0, 0, 0, 1);
    MaterialTextureSample textures = SampleNtcMaterial(uint2(i_position.xy), i_uv);
	// pbr shading here.
	float3 N = normalize(textures.normal.rgb * 2.0 - 1.0);
	N = normalize(mul(N, i_tbn));
	float3 V = normalize(g_BaseData.cameraPosition.xyz - i_worldPos.xyz);
	// point lights
    for (int i = 0; i < g_SceneData.numLights; i++)
    {
        PointLight light = g_SceneData.pointLights[i];
        float3 L = normalize(light.position.xyz - i_worldPos.xyz);
        float3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        // Cook-Torrance BRDF
        float roughness = textures.metalRoughOrSpecular.g;
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), textures.baseOrDiffuse.rgb, textures.metalRoughOrSpecular.r);
        float3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float denomD = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
        float D = alpha2 / (3.14159265 * denomD * denomD + 1e-5);
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G_V = NdotV / (NdotV * (1.0 - k) + k + 1e-5);
        float G_L = NdotL / (NdotL * (1.0 - k) + k + 1e-5);
        float G = G_V * G_L;
        float3 specular = (D * F * G) / (4.0 * NdotV * NdotL + 1e-5);
        // Lambertian diffuse
        float3 kD = (1.0 - F) * (1.0 - textures.metalRoughOrSpecular.r);
        float3 diffuse = kD * textures.baseOrDiffuse.rgb / 3.14159265;
        // Final shading
		float3 radiance = light.color.rgb * light.color.w;
		float dist = length(light.position.xyz - i_worldPos.xyz);
		radiance /= (dist * dist); // inverse square law
        o_color.rgb += (diffuse + specular) * radiance * NdotL;
    }
    // directional light
    {
        float3 L = normalize(-g_SceneData.dirLight.direction.xyz);
        float3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        // Cook-Torrance BRDF
        float roughness = textures.metalRoughOrSpecular.g;
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), textures.baseOrDiffuse.rgb, textures.metalRoughOrSpecular.r);
        float3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float denomD = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
        float D = alpha2 / (3.14159265 * denomD * denomD + 1e-5);
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G_V = NdotV / (NdotV * (1.0 - k) + k + 1e-5);
        float G_L = NdotL / (NdotL * (1.0 - k) + k + 1e-5);
        float G = G_V * G_L;
        float3 specular = (D * F * G) / (4.0 * NdotV * NdotL + 1e-5);
        // Lambertian diffuse
        float3 kD = (1.0 - F) * (1.0 - textures.metalRoughOrSpecular.r);
        float3 diffuse = kD * textures.baseOrDiffuse.rgb / 3.14159265;
        // Final shading
		float3 radiance = g_SceneData.dirLight.color.rgb * g_SceneData.dirLight.color.w;
		o_color.rgb += (diffuse + specular) * radiance * NdotL;
    }
    // Ambient term
	float ao = textures.occlusion.r;
    float3 ambient = float3(0.03, 0.03, 0.03) * textures.baseOrDiffuse.rgb * ao;
    o_color.rgb += ambient;
    o_color.a = 1.0;
    // Emissive term
    // o_color.rgb += textures.emissive.rgb;
	// Opacity
    // o_color.rgb = textures.baseOrDiffuse.rgb;
}
