#version 450
// layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(location = 0) in vec2 fragTexCoord;
void main() 
{
    //if (texture(baseColorSampler, fragTexCoord).a < 0.1) {
    //    discard;
    //}
}