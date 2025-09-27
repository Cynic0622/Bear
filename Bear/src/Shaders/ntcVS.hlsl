// HLSL Code

// 1. 资源绑定：Uniform Buffer -> Constant Buffer
//    - `layout(set=0, binding=0)` -> `register(b0, space0)`
cbuffer GlobalParams : register(b0, space0)
{
    matrix viewMatrix;
    matrix projMatrix;
};

// 2. 资源绑定：Push Constants -> 常规 Constant Buffer
//    (HLSL 的标准模型没有直接对应 Push Constants 的语法,
//     通常用一个高频更新的 Constant Buffer 模拟)
struct PerObjectPushConstants
{
    matrix model;
};
[[vk::push_constant]]
PerObjectPushConstants pushConstants;

// 3. 定义 VS 输入和 PS 输出的结构体
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


// 4. 主函数
VertexToPixel main(VertexInput input)
{
    VertexToPixel output;

    // 5. 矩阵乘法顺序调整
    // GLSL (列主序) 中 P*V*M，HLSL (行主序) 中是 M*V*P
    // 为了保持与 GLSL 一致的数学逻辑，假设 C++ 端传入的是转置后的矩阵
    // 或直接按 HLSL 风格编写：pos * M * V * P
    // matrix mvp = mul(mul(pushConstants.model, viewMatrix), projMatrix);
    // matrix modelView = mul(pushConstants.model, viewMatrix);
    // matrix mvp = mul(modelView, projMatrix);
    // matrix mvp = mul(mul(projMatrix, viewMatrix), pushConstants.model);
	matrix mvp = mul(projMatrix, mul(viewMatrix, pushConstants.model));
    output.position = mul(mvp, float4(input.position, 1.0));
    // 6. 坐标变换
    // output.position = mul(float4(input.position, 1.0), mvp);

    // 7. 数据传递
    output.texCoord = input.texCoord;

    return output;
}