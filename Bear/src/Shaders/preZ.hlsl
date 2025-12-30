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
    [[vk::location(3)]] float2 texCoord : TEXCOORD0;
};

struct VertexToPixel
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VertexToPixel main(VertexInput input)
{
    VertexToPixel output;
    // matrix mvp = mul(mul(pushConstants.model, viewMatrix), projMatrix);
	matrix mvp = mul(g_BaseData.projMat, mul(g_BaseData.viewMat, pushConstants.model));
    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoord = input.texCoord;

    return output;
}