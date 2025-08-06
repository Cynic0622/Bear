#version 450

layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;
void main() {
    outColor = texture(texSampler, fragTexCoord);
    // outColor = vec4(fragTexCoord.x, 1 - fragTexCoord.y, 0.0, 1.0);
    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}