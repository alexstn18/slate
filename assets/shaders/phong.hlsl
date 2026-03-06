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
};

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT       1

cbuffer Constants : register(b0)
{
    float4x4 NormalMatrix;
    float4x4 Model;
    float4x4 MVP;
    float3 CameraPos;
    uint LightCount;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D occlusionTexture : register(t2);
Texture2D emissionTexture : register(t3);

StructuredBuffer<Light> lights : register(t4);

SamplerState linearSampler : register(s0);

// @TODO: move specular params into Constants or material CBV
static const float SPECULAR_STRENGTH = 0.5f;
static const float SPECULAR_GLOSSINESS = 0.25f;
static const float AMBIENT_INTENSITY = 0.05f;

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
    return pow(sRGB, 2.2f);
}

float3 LinearToSRGB(float3 l)
{
    return pow(l, 1.0f / 2.2f);
}

float3 ACESFilmic(float3 x)
{
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
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

float4 LightingCalculation(PSInput input)
{
    float3 N = ComputeTBNNormal(input);
    float3 viewDir = normalize(CameraPos - input.WorldPos);
    float specExp = exp2(SPECULAR_GLOSSINESS * 8) + 2;

    float3 accumulated = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < LightCount; ++i)
    {
        Light light = lights[i];

        float3 L = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (light.Type == LIGHT_TYPE_DIRECTIONAL)
        {
            L = normalize(-light.Direction);
        }
        else // LIGHT_TYPE_POINT
        {
            float3 toLight = light.Position - input.WorldPos;
            float dist = length(toLight);
            L = normalize(toLight);

            attenuation = saturate(1.0f - dist / light.Range);
            attenuation *= attenuation;

            if (attenuation <= 0.01f)
                continue;
        }

        float3 ambient = light.Color * AMBIENT_INTENSITY;

        float lambert = dot(N, L);
        float diff = max(lambert, 0.0f);
        float3 diffuse = diff * light.Intensity * light.Color;

        float3 halfway = normalize(L + viewDir);
        float spec = pow(max(dot(N, halfway), 0.0f) * (lambert > 0), specExp) * SPECULAR_GLOSSINESS;
        float3 specular = spec * light.Color;

        accumulated += (ambient + diffuse + specular) * attenuation;
    }

    float3 emission = SRGBToLinear(emissionTexture.Sample(linearSampler, input.TexCoord).rgb);

    return float4(accumulated + emission, 1.0f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 albedo = SRGBToLinear(albedoTexture.Sample(linearSampler, input.TexCoord).rgb);

    float4 lighting = LightingCalculation(input);
    float3 color = lighting.rgb * albedo;

    color = ACESFilmic(color);
    color = LinearToSRGB(color);

    return float4(color, 1.0f);
}