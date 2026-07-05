struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : WORLDPOS;
    float2 TexCoord : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 BiTangent : BITANGENT;
};

struct Light
{
    float3 Color;
    float Intensity;
    float3 Position;
    float Range;
    float3 Direction;
    uint Type;
    float SpecularGlossiness;
    float AmbientIntensity;
};

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT       1

#define TONEMAP_TYPE_REINHARD  0
#define TONEMAP_TYPE_FILMIC    1
#define TONEMAP_TYPE_ACES      2

cbuffer ModelConstants : register(b0)
{
    float4x4 NormalMatrix;
    float4x4 Model;
    float4x4 MVP;
    float3 CameraPos;
};

cbuffer GeneralData : register(b1)
{
    uint LightCount;
    uint TonemapMode;
    float GammaCorrection;
    float Exposure;
};

cbuffer Material : register(b2)
{
    float4 AlbedoFactor;
    float3 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D occlusionTexture : register(t2);
Texture2D emissionTexture : register(t3);

StructuredBuffer<Light> lights : register(t4);

SamplerState linearSampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(MVP, float4(input.Position, 1.0f));
    output.Normal = mul((float3x3) NormalMatrix, input.Normal);
    output.WorldPos = mul(Model, float4(input.Position, 1.0f)).xyz;
    output.TexCoord = input.TexCoord;
    output.Tangent = mul((float3x3) NormalMatrix, input.Tangent);
    output.BiTangent = mul((float3x3) NormalMatrix, input.BiTangent);
    return output;
}

float3 SRGBToLinear(float3 sRGB)
{
    return pow(sRGB, GammaCorrection);
}

float3 LinearToSRGB(float3 l)
{
    return pow(l, 1.0f / GammaCorrection);
}

float3 Reinhard(float3 x)
{
    return x / (x + float3(1.0, 1.0, 1.0));
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilmic(float3 x)
{
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// https://www.shadertoy.com/view/XsGfWV
float3 ACES(float3 color)
{
    const float3x3 ACESInputMat = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );
    const float3x3 ACESOutputMat = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602
    );
    color = mul(ACESInputMat, color);
    color = (color * (color + 0.0245786f) - 0.000090537f) /
            (color * (0.983729f * color + 0.4329510f) + 0.238081f);
    return mul(ACESOutputMat, color);
}

float3 ComputeTBNNormal(PSInput input)
{
    float3 T = normalize(input.Tangent);
    float3 Nv = normalize(input.Normal);
    T = normalize(T - dot(T, Nv) * Nv); // Gram-Schmidt
    float3 B = cross(Nv, T);

    float3x3 TBN =
    {
        T.x, B.x, Nv.x,
        T.y, B.y, Nv.y,
        T.z, B.z, Nv.z
    };
    
    // unpack normals from [0, 1] back to [-1, 1]
    float3 tangentNormal = normalTexture.Sample(linearSampler, input.TexCoord).rgb * 2.0f - 1.0f;
    return normalize(mul(TBN, tangentNormal));
}

float3 F_schlick(in float3 f0, in float LoH)
{
    return (f0 + (1. - f0) * pow(1. - LoH, 5.));
}

static const float PI = 3.14159265359f;
static const float INVPI = 1.0f / PI;

float DistributionGGX(float NoH, float a)
{
    float a2 = a * a;
    float NdotH2 = NoH * NoH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.) + 1.);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1. - k) + k;
    return nom / denom;
}

float GeometrySmith(float NoV, float NoL, float k)
{
    float ggx1 = GeometrySchlickGGX(NoV, k);
    float ggx2 = GeometrySchlickGGX(NoL, k);
    return ggx1 * ggx2;
}

float3 CalculateFRough(float3 F0, float3 V, float3 N, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float oneMinusRough = 1. - roughness;
    float3 fresnel = F0 + (max(float3(oneMinusRough, oneMinusRough, oneMinusRough), F0) - F0) * pow(1. - NdotV, 5.);
    return fresnel;
}

float4 LightingCalculation(PSInput input)
{
    float3 N = ComputeTBNNormal(input);
    float3 V = normalize(CameraPos - input.WorldPos);

    float3 albedo = SRGBToLinear(
        albedoTexture.Sample(linearSampler, input.TexCoord).rgb
    ) * AlbedoFactor.rgb;

    float3 orm = occlusionTexture.Sample(linearSampler, input.TexCoord).rgb;

    float ao = orm.r;

    float roughness = max(
        orm.g * RoughnessFactor,
        0.04f
    );

    float metallic =
    orm.b * MetallicFactor;

    // reflectivity
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);

    float3 accumulated = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < LightCount; ++i)
    {
        Light light = lights[i];

        float3 L;
        float attenuation = 1.0f;

        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            L = normalize(-light.Direction);
        }
        else
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);

            if (dist > light.Range)
                continue;

            L = toLight / dist;

            attenuation = saturate(1.0f - dist / light.Range);
            attenuation *= attenuation;
        }

        float3 H = normalize(V + L);

        float NoV = saturate(dot(N, V));
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float LoH = saturate(dot(L, H));

        if (NoL <= 0.0f)
            continue;

        float alpha = roughness * roughness;

        float D = DistributionGGX(NoH, alpha);

        float k = (roughness + 1.0f);
        k = (k * k) / 8.0f;

        float G = GeometrySmith(NoV, NoL, k);

        float3 F = F_schlick(F0, LoH);

        float3 numerator = D * G * F;
        float denominator = max(4.0f * NoV * NoL, 0.001f);

        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = (1.0f - kS) * (1.0f - metallic);

        float3 diffuse = kD * albedo * INVPI;

        float3 radiance =
            light.Color *
            light.Intensity *
            attenuation;

        accumulated += (diffuse + specular) * radiance * NoL;
    }

    // replace with IBL
    float3 ambient = 0.03f * albedo * ao;

    float3 emission =
        SRGBToLinear(
            emissionTexture.Sample(linearSampler, input.TexCoord).rgb
        ) * EmissiveFactor;

    return float4(accumulated + ambient + emission, 1.0f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 lighting = LightingCalculation(input);
    float3 color = lighting.rgb;

    color *= Exposure;

    if (TonemapMode == TONEMAP_TYPE_REINHARD)
    {
        color = Reinhard(color);
    }
    
    if (TonemapMode == TONEMAP_TYPE_FILMIC)
    {
        color = ACESFilmic(color);
    }
    
    if (TonemapMode == TONEMAP_TYPE_ACES)
    {
        color = ACES(color);
    }
    
    color = LinearToSRGB(color);

    return float4(color, 1.0f);
}