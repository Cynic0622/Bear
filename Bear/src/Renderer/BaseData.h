#ifdef __cplusplus
    // ================= C++=================
	#include <glm/glm.hpp>
	#pragma once
	using float4 = glm::vec4;
	using float3 = glm::vec3;
	using float2 = glm::vec2;
	using uint = uint32_t;
	using float4x4 = glm::mat4;

	#define STRUCT(Name) struct Name

#elif defined(HLSL) || defined(__HLSL_VERSION)
    // ================= HLSL =================
    // #pragma pack_matrix(row_major) 

	#define STRUCT(Name) struct Name

#else
    // ================= GLSL =================

	#define float4 vec4
	#define float3 vec3
	#define float2 vec2
	#define float4x4 mat4
	#define uint uint

	#define STRUCT(Name) struct Name
#endif

// ================= Base Data =================
#define MAX_POINT_LIGHTS 5

STRUCT(DirectionalLight)
{
    float4 color;
    float4 direction; // xyz: dir, w: padding
};

STRUCT(PointLight)
{
	float4 color; // w = intensity
	float4 position;
};

STRUCT(SceneData)
{
	PointLight pointLights[MAX_POINT_LIGHTS];
    DirectionalLight dirLight;
    int numLights;
};

STRUCT(BaseData)
{
	float4x4 viewMat;
	float4x4 projMat;
	float4 cameraPosition;
	uint frameIndex;
};