// Include the constants header unconditionally so that NTC_NETWORK_UNKNOWN is always defined
#include "libntc/shaders/InferenceConstants.h"
#include "libntc/shaders/Inference.hlsli"
#include "NtcChannelMapping.h"
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

[[vk::binding(0, 1)]] ByteAddressBuffer t_InputFile; // the latent data.

[[vk::binding(1, 1)]] ByteAddressBuffer t_WeightBuffer;// the neural network weights.

[[vk::binding(2, 1)]] ConstantBuffer<NtcTextureSetConstants> g_NtcMaterial;

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
    HashBasedRNG rng = HashBasedRNG::Create2D(pixelPosition, 1025);
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
	// in bool i_isFrontFace : SV_IsFrontFace, // whether the primitive is front-facing or back-facing.
	VK_LOCATION_INDEX(0, 0) out float4 o_color : SV_Target0 // the output color of this pixel.
)
{
    MaterialTextureSample textures = SampleNtcMaterial(uint2(i_position.xy), i_uv);
	o_color = float4(textures.baseOrDiffuse.rgb, textures.opacity.r);
    // uint first_weight = t_WeightBuffer.Load<uint>(0);
    // float color = float(first_weight & 0xFF) / 255.0;
    // o_color = float4(color.xxx, 1.0);
	// o_color = float4(i_uv.x, i_uv.y, 0.0, 1.0); // debug output
}
