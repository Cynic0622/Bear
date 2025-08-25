#version 450
#define MAX_FRAGMENT_COUNT 128

struct Node
{
    vec4 color;
    float depth;
    uint next;
};

layout (location = 0) out vec4 outFragColor;
layout(set = 0, binding = 0) buffer LinkedListSBO
{
    Node nodes[];
};
layout (set = 0, binding = 1, r32ui) uniform uimage2D headIndexImage;

void main()
{
    Node fragments[MAX_FRAGMENT_COUNT];
    int count = 0;

    uint nodeIdx = imageLoad(headIndexImage, ivec2(gl_FragCoord.xy)).r;

    while (nodeIdx != 0xffffffff && count < MAX_FRAGMENT_COUNT)
    {
        fragments[count] = nodes[nodeIdx];
        nodeIdx = fragments[count].next;
        ++count;
    }
    
    // Do the insertion sort
    for (uint i = 1; i < count; ++i)
    {
        Node insert = fragments[i];
        uint j = i;
        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j-1];
            --j;
        }
        fragments[j] = insert;
    }

    // Do blending
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < count; ++i)
    {
        color.rgb = color.rgb * (1.0 - fragments[i].color.a) + fragments[i].color.rgb * fragments[i].color.a;
        color.a = color.a * (1.0 - fragments[i].color.a) + fragments[i].color.a;
    }
    if (count > 0)
    {
        outFragColor = color;
    }
    else
    {
        discard;
    }
}