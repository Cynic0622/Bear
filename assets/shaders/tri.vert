#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform GlobalParams {
    mat4 viewMatrix;
    mat4 projMatrix;

    vec4 cameraPosition;
    vec4 lightPositions[10];
    vec4 lightColors[10];
    int lightCount;
} globalParamsData;

layout(push_constant) uniform PerObject {
    mat4 modelMatrix;
} perObjectData;

// pass through to fragment shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out mat3 TBN;

void main() {
    gl_Position = globalParamsData.projMatrix * globalParamsData.viewMatrix * perObjectData.modelMatrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;

    fragPos = vec3(perObjectData.modelMatrix * vec4(inPosition, 1.0));
    viewDir = -normalize(fragPos - globalParamsData.cameraPosition.xyz);
    vec3 N = normalize(mat3(transpose(inverse(perObjectData.modelMatrix))) * inNormal);
    vec3 T = normalize(mat3(transpose(inverse(perObjectData.modelMatrix))) * inTangent);
    vec3 B = cross(N, T);                     // bitangent
    TBN = mat3(T, B, N);
    // TBN = mat3(normalize(inNormal), cross(normalize(inNormal), vec3(0.0, 1.0, 0.0)), vec3(0.0, 1.0, 0.0));
}