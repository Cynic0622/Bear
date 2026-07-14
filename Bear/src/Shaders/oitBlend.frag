#version 450
#define MAX_NODES 4

struct Node
{
    vec4 color;
    float depth;
};

layout (location = 0) out vec4 outFragColor;
layout(set = 0, binding = 0) buffer NodeBuffer
{
    Node nodes[];
};
layout (set = 0, binding = 1, r32ui) uniform uimage2D pixelCounter;

void main()
{
    ivec2 dim = imageSize(pixelCounter);
    uint count = imageLoad(pixelCounter, ivec2(gl_FragCoord.xy)).r;
    uint pixelBase = (uint(gl_FragCoord.y) * uint(dim.x) + uint(gl_FragCoord.x)) * MAX_NODES;

    Node fragments[MAX_NODES];

    for (uint i = 0; i < min(count, MAX_NODES); ++i)
    {
        fragments[i] = nodes[pixelBase + i];
    }

    // insertion sort by depth (back-to-front)
    for (uint i = 1; i < min(count, MAX_NODES); ++i)
    {
        Node insert = fragments[i];
        uint j = i;
        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j - 1];
            --j;
        }
        fragments[j] = insert;
    }

    // blend (premultiplied alpha over)
    vec4 color = vec4(0.0);
    uint n = min(count, MAX_NODES);
    for (uint i = 0; i < n; ++i)
    {
        color.rgb = color.rgb * (1.0 - fragments[i].color.a) + fragments[i].color.rgb * fragments[i].color.a;
        color.a = color.a * (1.0 - fragments[i].color.a) + fragments[i].color.a;
    }

    if (n > 0)
    {
        outFragColor = color;
    }
    else
    {
        discard;
    }
}