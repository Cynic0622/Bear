#include "BaseData.h"
[[vk::binding(0, 0)]] ConstantBuffer<BaseData> g_BaseData;

struct PerObjectPushConstants
{
    matrix model;
};
[[vk::push_constant]]
PerObjectPushConstants pushConstants;

struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(1)]] float3 normal : NORMAL0;
	[[vk::location(2)]] float3 tangent : TANGENT0;
    [[vk::location(3)]] float2 texCoord : TEXCOORD0;
};

struct VertexToPixel
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
	float4 worldPos : TEXCOORD1;
	float3x3 TBN : TEXCOORD2;
};

VertexToPixel main(VertexInput input)
{
    VertexToPixel output;
	matrix mvp = mul(g_BaseData.projMat, mul(g_BaseData.viewMat, pushConstants.model));
    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoord = input.texCoord;
	output.worldPos = mul(pushConstants.model, float4(input.position, 1.0));
	float3 N = normalize(mul((float3x3)pushConstants.model, input.normal));
    // float3 N = normalize(mul(input.normal, (float3x3)pushConstants.model));
	float3 T = normalize(mul((float3x3)pushConstants.model, input.tangent));
	// float3 T = normalize(mul(input.tangent, (float3x3)pushConstants.model));
	float3 B = cross(N, T);
	output.TBN = float3x3(T, B, N);

    return output;
}