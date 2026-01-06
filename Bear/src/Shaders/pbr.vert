#version 450
#include "..\\Renderer\\BaseData.h"
layout(set = 0, binding = 0) uniform baseDataBlock { BaseData g_BaseData; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;  // xyz + w(handedness)
layout(location = 3) in vec2 inTexCoord;

layout(push_constant) uniform PerObject { mat4 modelMat; } perObjectData;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitangent;

void main() {
    mat4 mvp = g_BaseData.projMat * g_BaseData.viewMat * perObjectData.modelMat;
    gl_Position = mvp * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    // fragPos 是世界坐标
    fragPos = vec3(perObjectData.modelMat * vec4(inPosition, 1.0));
    viewDir = normalize(g_BaseData.cameraPosition.xyz - fragPos);

    // 1. 计算法线矩阵 (用于 Normal)
    mat3 normalMat = mat3(transpose(inverse(perObjectData.modelMat)));
    vec3 N = normalize(normalMat * inNormal);

    // 2. 计算切线 (FIX: 使用 Model Matrix，而不是 Normal Matrix)
    // 只取 Model Matrix 的 3x3 旋转缩放部分
    vec3 T = normalize(mat3(perObjectData.modelMat) * inTangent.xyz);
    
    // Gram-Schmidt 正交化 (重新让 T 垂直于 N)
    T = normalize(T - N * dot(T, N));

    // 3. 计算副切线 (Bitangent)
    float handedness = inTangent.w;
    vec3 B = normalize(cross(N, T)) * handedness;

    outNormal = N;
    outBitangent = B;
    outTangent = T;
}