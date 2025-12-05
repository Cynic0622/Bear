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

layout(location = 0) out vec2 fragTexCoord;
void main() {
    mat4 mvp = globalParamsData.projMatrix * globalParamsData.viewMatrix * pushConstants.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}