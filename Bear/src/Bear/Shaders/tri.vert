#version 450

// 硬编码的三角形顶点数据
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5),   // 顶部
    vec2(-0.5,  0.5),   // 左下
    vec2( 0.5,  0.5)    // 右下
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),  // 红色
    vec3(0.0, 1.0, 0.0),  // 绿色
    vec3(0.0, 0.0, 1.0)   // 蓝色
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}