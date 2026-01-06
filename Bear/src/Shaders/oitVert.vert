#version 450
#include "..\\Renderer\\BaseData.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent; // 注意：必须用 vec4 读取 w (手性)
layout(location = 3) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform baseDataBlock { BaseData g_BaseData; };
layout(push_constant) uniform PerObject { mat4 modelMatrix; } perObjectData;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 viewDir;

layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitangent;

void main() {
    gl_Position = g_BaseData.projMat * g_BaseData.viewMat * perObjectData.modelMatrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;

    fragPos = vec3(perObjectData.modelMatrix * vec4(inPosition, 1.0));
    viewDir = -normalize(fragPos - g_BaseData.cameraPosition.xyz); // 注意：通常 ViewDir 指向相机，如果是 -fragPos+camPos 则是指向相机

    // 1. 计算法线 N (使用法线矩阵)
    mat3 normalMatrix = mat3(transpose(inverse(perObjectData.modelMatrix)));
    vec3 N = normalize(normalMatrix * inNormal);

    // 2. 计算切线 T (修改点：使用模型矩阵！)
    vec3 T = normalize(mat3(perObjectData.modelMatrix) * inTangent.xyz);

    // 3. Gram-Schmidt 正交化 (修改点：强制 T 垂直于 N)
    T = normalize(T - N * dot(T, N));

    // 4. 计算副切线 B (修改点：使用 w 分量处理镜像)
    // 如果没有 w 分量，镜像UV的模型法线会反向
    vec3 B = cross(N, T) * inTangent.w;

    outNormal = N;
    outTangent = T;
    outBitangent = B;
}