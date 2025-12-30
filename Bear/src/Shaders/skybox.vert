#version 450

#include "..\Renderer\BaseData.h"
vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  3.0),
	vec2( 3.0, -1.0)
);
layout(location = 0) out vec3 outPosition;
void main() {
	outPosition = vec3(positions[gl_VertexIndex], 1.0);
	gl_Position = vec4(outPosition, 1.0);
}