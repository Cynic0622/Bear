#include "BaseData.h"
[[vk::binding(0, 0)]] ConstantBuffer<BaseData> g_BaseData;

struct PerObjectData
{
	matrix model;
};
[[vk::binding(2, 1)]] StructuredBuffer<PerObjectData> g_PerObject;

struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(3)]] float2 texCoord : TEXCOORD0;
    uint instanceId : SV_InstanceID;
};

struct VertexToPixel
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VertexToPixel main(VertexInput input)
{
    VertexToPixel output;
	matrix modelMat = g_PerObject[input.instanceId].model;
	matrix mvp = mul(g_BaseData.projMat, mul(g_BaseData.viewMat, modelMat));
    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoord = input.texCoord;

    return output;
}
