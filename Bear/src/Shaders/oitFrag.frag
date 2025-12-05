#version 450

struct Node
{
    vec4 color;
    float depth;
    uint next;
};
struct LightData
{
    vec4 position;
    vec4 colorIntensity; // xyz: color, w: intensity
};
layout(std140, set = 0, binding = 0) uniform GlobalParams {
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 cameraPosition;
    LightData lightsData[50];
    int lightCount;
} globalParamsData;
layout(set = 1, binding = 0) uniform Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3 emissiveFactor;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 4) uniform sampler2D occlusionSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in mat3 TBN; // tangent, bitangent, normal matrix

layout (set = 2, binding = 0) buffer LinkedListSBO
{
    Node nodes[];
};
layout (set = 2, binding = 1) buffer AtomicCounter
{
    uint counter;
    uint maxNodeCount;
};

layout (set = 2, binding = 2, r32ui) uniform uimage2D headIndexImage;

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);

    vec3 albedo = baseColor.rgb * materialData.baseColorFactor.rgb;
    float alpha = baseColor.a * materialData.baseColorFactor.a;
    if (baseColor.a < 0.2) discard;

    vec3 normal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
    normal.xy *= materialData.normalScale;
    normal = normalize(normal);
    vec3 N = normalize(TBN * normal);

    vec3 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rgb;
    float metallic = clamp(materialData.metallicFactor * metallicRoughness.b, 0.0, 1.0);
    float roughness = clamp(materialData.roughnessFactor * metallicRoughness.g, 0.04, 1.0);

    float ao = texture(occlusionSampler, fragTexCoord).r * materialData.occlusionStrength;
    vec3 emissive = texture(emissiveSampler, fragTexCoord).rgb * materialData.emissiveFactor;

    vec3 V = normalize(viewDir);
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    // pbr render cook-torrance
    for (int i = 0; i < globalParamsData.lightCount; ++i)
    {
        vec3 lightPos = globalParamsData.lightsData[i].position.xyz;
        vec3 L = normalize(lightPos - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos - fragPos);
        float attenuation = 1.0 / (distance * distance); // simple quadratic attenuation
        vec3 radiance = globalParamsData.lightsData[i].colorIntensity.rgb * globalParamsData.lightsData[i].colorIntensity.a * attenuation;

        // blin-phong
        float shininess = clamp(2.0 / (roughness * roughness) - 2.0, 1.0, 4096.0);
        vec3 spec = pow(max(dot(N, H), 0.0), shininess) * radiance;
        vec3 diff = max(dot(N, L), 0.0) * radiance;

        // kS is specular, kD is diffuse (energy conservation)
        vec3 kS = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        // Lambert diffuse
        vec3 diffuse = kD * diff;
        vec3 specular = kS * spec;
        Lo += diffuse + specular;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo + emissive;
    color = color / (color + vec3(1.0));
    // vec4 color = baseColor * materialData.baseColorFactor;
    uint nodeIdx = atomicAdd(counter, 1);

    if (nodeIdx < maxNodeCount)
    {
        uint prevHeadIdx = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIdx);
        // Write the fragment data to the linked list
        nodes[nodeIdx].color = vec4(color, alpha);
        // nodes[nodeIdx].color = color;
        nodes[nodeIdx].next = prevHeadIdx;
        nodes[nodeIdx].depth = gl_FragCoord.z;
    }
}