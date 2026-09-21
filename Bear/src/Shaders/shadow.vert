#version 450

#include "..\\Renderer\\BaseData.h"

layout(set = 0, binding = 0) uniform sceneDataBlock { SceneData g_SceneData; };
layout(set = 0, binding = 2) readonly buffer PerObjectSSBO { mat4 models[]; } g_PerObject;

layout(push_constant) uniform PushConstants
{
    mat4 lightViewProj;
    float texelWorld;
    float normalOffset;
} g_PC;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main()
{
    mat4 modelMat = g_PerObject.models[gl_InstanceIndex];
    vec4 world = modelMat * vec4(inPosition, 1.0);

    // normal offset: shifts the caster along its normal to reduce shadow acne
    vec3 normal = normalize(mat3(modelMat) * inNormal);
    world.xyz += normal * (g_PC.texelWorld * g_PC.normalOffset);

    gl_Position = g_PC.lightViewProj * world;
}
