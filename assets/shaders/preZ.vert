#version 450

layout(std140, set = 0, binding = 0) uniform GlobalParams {
    mat4 viewMatrix;
    mat4 projMatrix;
} globalParamsData;

layout(push_constant) uniform PerObjectPushConstants {
    mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec2 inTexCoord;

// 输出（最小化）
layout(location = 0) out vec2 fragTexCoord;  // 仅用于alpha test

void main() {
    mat4 mvp = globalParamsData.projMatrix * globalParamsData.viewMatrix * pushConstants.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    
    // 只传递纹理坐标用于alpha test
    fragTexCoord = inTexCoord;
}