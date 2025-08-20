#version 450
layout(set = 1, binding = 0) uniform Material {
    vec4 baseColorFactor;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(location = 0) in vec2 fragTexCoord;
void main() {
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);
    if (baseColor.a < 0.2) {
        discard;
    }
}