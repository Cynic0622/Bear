cbuffer GlobalParams : register(b0, space0)
{
    matrix viewMatrix;
    matrix projMatrix;
};

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
	matrix mvp = mul(projMatrix, mul(viewMatrix, pushConstants.model));
    output.position = mul(mvp, float4(input.position, 1.0));
    output.texCoord = input.texCoord;

    return output;
}