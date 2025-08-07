#version 450

// 接收来自顶点缓冲的属性
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(set = 0, binding = 0) uniform UBO{
    mat4 viewMatrix;  // 视图矩阵
    mat4 projMatrix;  // 投影矩阵
} uboData;
layout(push_constant) uniform PerObject {
    mat4 modelMatrix; // 模型矩阵
} perObjectData;

// pass through to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = uboData.projMatrix * uboData.viewMatrix * perObjectData.modelMatrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}