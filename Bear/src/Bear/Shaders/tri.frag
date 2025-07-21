#version 450
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec4 inColor;

void main() {
	// Set the fragment color to the interpolated color from the vertex shader
	FragColor = inColor;
}