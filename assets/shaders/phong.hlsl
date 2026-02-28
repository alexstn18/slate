struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : WORLDPOS;
    float2 TexCoord : TEXCOORD;
};

struct LightInfo
{
    float3 Color;
    float3 Position;
    float3 Direction;
    float  Intensity;
    uint   Type;
    // float  Type;
};

#define TYPE_DIRECTIONAL 0
#define TYPE_POINT 1

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 NormalMatrix;
    float4x4 Model;
    float4x4 MVP;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(MVP, float4(input.Position, 1.0f));
    output.Normal = mul((float3x3) NormalMatrix, input.Normal);
    output.WorldPos = mul(Model, float4(input.Position, 1.0f)).xyz;
    output.TexCoord = input.TexCoord;
    return output;
}

float4 LightingCalculation(PSInput input)
{
    // @TODO: change this to be included in the constant buffer
    // once you have a proper camera
    const float3 CAMERA_POS = float3(0.0f, 0.0f, 5.0f);
    const float SPECULAR_STRENGTH = 0.5f;
    const float SPECULAR_GLOSSINESS = 0.25f;
    const float AMBIENT_INTENSITY = 0.05f;
    
    // @TODO: point light attenuation values, move to CBV
    const float LIGHT_RANGE = 10000.0f;
    
    
    // @TODO: move to using a CBV upload buffer instead of hardcoding stuff
    LightInfo lightInfo;
    lightInfo.Type = TYPE_DIRECTIONAL;
    lightInfo.Color = float3(1.0f, 0.95f, 0.8f);
    lightInfo.Intensity = 1.0f;
    
    float3 N = normalize(input.Normal);
    float3 viewDir = normalize(CAMERA_POS - input.WorldPos);
    float3 L = float3(0.0f, 0.0f, 0.0f);
    float specularExponent = exp2(SPECULAR_GLOSSINESS * 8) + 2;
    float attenuation = 1.0f;
    if(lightInfo.Type == TYPE_DIRECTIONAL)
    {
        lightInfo.Direction = float3(15.0f, 0.0f, -10.0f);
        L = normalize(-lightInfo.Direction);
    }
    else
    {
        lightInfo.Position = float3(3.0f, 3.0f, 5.0f);
        L = normalize(lightInfo.Position - input.WorldPos);
        
        float dist = length(lightInfo.Position - input.WorldPos);
        attenuation = saturate(1.0f - dist / LIGHT_RANGE);
        attenuation *= attenuation;
    }
    
    float4 ambient = float4(lightInfo.Color * AMBIENT_INTENSITY, 1.0f);
    float lambertian = dot(N, L);
    float diff = max(lambertian, 0.0f);
    float4 diffuse = float4(diff * lightInfo.Intensity * lightInfo.Color, 1.0f);
    float3 halfwayDir = normalize(L + viewDir);
    // float3 reflectDir = reflect(-lightDir, input.Normal); // this is used in phong, not blinn-phong
    float spec = max(dot(N, halfwayDir), 0.0f) * (lambertian > 0);
    spec = pow(spec, specularExponent) * SPECULAR_GLOSSINESS; // * gloss is an approximation for PBR-like i think?
    float4 specular = float4(spec * lightInfo.Color, 1.0f);
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    float4 final = ambient + diffuse + specular;
    return final;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 tex = albedoTexture.Sample(linearSampler, input.TexCoord);
    return LightingCalculation(input) * tex;
}