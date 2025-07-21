#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 outColor; // Output color to fragment shader

void main() {
	gl_Position = vec4(position, 1.0);
	gl_PointSize = 10.0; // Set point size for rendering
	// Pass color to fragment shader
	outColor = vec4(color, 1.0);
}