#version 450
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
layout(location = 0) out vec4 outColor;

// PBR helpers
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

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
        // attenuation = 1.0 / distance; // linear attenuation
        vec3 radiance = globalParamsData.lightsData[i].colorIntensity.rgb * globalParamsData.lightsData[i].colorIntensity.a * attenuation;

        // cook-torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, (roughness+1.0)*(roughness+1.0)/8.0);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denom;

        float NdotL = max(dot(N, L), 0.0);

        // kS is specular, kD is diffuse (energy conservation)
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        // Lambert diffuse
        vec3 diffuse = kD * albedo / 3.14159265;

        Lo += (diffuse + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    // ambient = vec3(0.2, 0.2, 0.2) * albedo * ao;
    vec3 color = ambient + Lo + emissive;

    color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0 / 2.2));
    // Combine the textures and material properties
    outColor = vec4(color, alpha);
    // outColor = vec4(albedo, 1.0);
}