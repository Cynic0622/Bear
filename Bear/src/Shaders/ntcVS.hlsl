#include "BaseData.h"
[[vk::binding(0, 0)]] ConstantBuffer<BaseData> g_BaseData;

// Per-object model matrices, the same set 1 / binding 2 storage buffer that pbr.vert reads.
// This pass draws with DrawIndexedIndirectCount (firstInstance = object index), so the model
// matrix cannot be delivered through push constants - it has to be fetched per instance here.
[[vk::binding(2, 1)]] StructuredBuffer<float4x4> g_PerObject;

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

VertexToPixel main(VertexInput input, uint instanceIndex : SV_InstanceID)
{
    VertexToPixel output;

    // Read the matrix as a matrix and let the compiler deal with its layout: dxc packs the
    // element row-major and emits the matching multiply, which yields the same components as
    // pbr.vert's "mat4 models[]; models[gl_InstanceIndex] * vec4(pos, 1)". Hand-rolling the
    // multiply from separate float4 columns is where the transpose mistakes come from.
    const float4x4 model = g_PerObject[instanceIndex];
    const float4 worldPos = mul(model, float4(input.position, 1.0));

    output.position = mul(g_BaseData.projMat, mul(g_BaseData.viewMat, worldPos));
    output.texCoord = input.texCoord;
    output.worldPos = worldPos;

    // Directions use w = 0 so the translation drops out (same as before: no inverse transpose).
    const float3 N = normalize(mul(model, float4(input.normal, 0.0)).xyz);
    const float3 T = normalize(mul(model, float4(input.tangent, 0.0)).xyz);
    const float3 B = cross(N, T);
    output.TBN = float3x3(T, B, N);

    return output;
}
