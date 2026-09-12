#version 450
#include "..\\Renderer\\BaseData.h"
layout(set = 0, binding = 0) uniform baseDataBlock { BaseData g_BaseData; };

layout(set = 1, binding = 2) readonly buffer PerObjectSSBO {
    mat4 models[];
} g_PerObject;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;  // xyz + w(handedness)
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitangent;

void main() {
    mat4 modelMat = g_PerObject.models[gl_InstanceIndex];
    mat4 mvp = g_BaseData.projMat * g_BaseData.viewMat * modelMat;
    gl_Position = mvp * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragPos = vec3(modelMat * vec4(inPosition, 1.0));
    viewDir = normalize(g_BaseData.cameraPosition.xyz - fragPos);

    mat3 normalMat = mat3(transpose(inverse(modelMat)));
    vec3 N = normalize(normalMat * inNormal);

    vec3 T = normalize(mat3(modelMat) * inTangent.xyz);
    T = normalize(T - N * dot(T, N));

    float handedness = inTangent.w;
    vec3 B = normalize(cross(N, T)) * handedness;

    outNormal = N;
    outBitangent = B;
    outTangent = T;
}
