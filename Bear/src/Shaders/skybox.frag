#version 450
#include "..\Renderer\BaseData.h"
layout(location = 0) out vec4 outFragColor;
layout(set = 0, binding = 0) uniform baseDataBlock {
	BaseData g_BaseData;
};
layout(set = 1, binding = 0) uniform sampler2D SkyboxSampler;
layout(location = 0) in vec3 inPosition;
vec3 ACESFilm(vec3 x)
{
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0
    );
}

void main()
{
// Get the view direction from the fragment position
	mat4 invProj = inverse(g_BaseData.projMat);
	vec4 view = invProj * vec4(inPosition, 1.0);
	view /= view.w;
	mat3 invView = mat3(inverse(g_BaseData.viewMat));
	vec3 worldDir = normalize(invView * view.xyz);
	vec3 viewDir = worldDir;
	// Sample the skybox texture using the view direction
	vec4 skyboxColor = texture(SkyboxSampler, vec2(
		atan(viewDir.z, viewDir.x) / (2.0 * 3.14159265) + 0.5,
		asin(viewDir.y) / 3.14159265 + 0.5));
	skyboxColor.rgb = skyboxColor.rgb * exp2(-2.0);
	outFragColor = skyboxColor;
	outFragColor.rgb = ACESFilm(outFragColor.rgb);
	// depth set far plane
	gl_FragDepth = 1.0;
}
