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
    float2 TexCoord : TEXCOORD;
};

struct LightInfo
{
    float3 Color;
    float3 Position;
    float3 Direction;
    float Intensity;
};

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 NormalMatrix;
    float4x4 MVP;
    // LightInfo lightInfo;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(MVP, float4(input.Position, 1.0f));
    output.Normal = mul((float3x3) NormalMatrix, input.Normal);
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.Normal);
    
    // @TODO: change this to be included in the constant buffer
    // once you have a proper camera
    const float3 CAMERA_POS = float3(0.0f, 0.0f, 5.0f);
    const float SPECULAR_STRENGTH = 0.5f;
    const float AMBIENT_INTENSITY = 0.05f;
    ///
    // @TODO: move to using a CBV upload buffer instead of hardcoding stuff
    LightInfo lightInfo;
    lightInfo.Color = float3(1.0f, 0.95f, 0.8f);
    lightInfo.Position = float3(3.0f, 3.0f, 5.0f);
    lightInfo.Intensity = 1.0f;
    ///
    float4 tex = albedoTexture.Sample(linearSampler, input.TexCoord);
    float4 ambient = float4(lightInfo.Color * AMBIENT_INTENSITY, 1.0f);
    float3 lightDir = normalize(lightInfo.Position - input.Position.xyz);
    float diff = max(dot(normal, lightDir), 0.0f);
    float4 diffuse = float4(diff * lightInfo.Intensity * lightInfo.Color, 1.0f);
    float3 viewDir = normalize(CAMERA_POS - input.Position.xyz);
    float3 halfwayDir = normalize(lightDir + viewDir);
    // float3 reflectDir = reflect(-lightDir, input.Normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), 16.0f);
    float4 specular = float4(SPECULAR_STRENGTH * spec * lightInfo.Color, 1.0f);
    return (ambient + diffuse + specular)  * tex;
}