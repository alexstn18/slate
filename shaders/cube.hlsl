struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 MVP;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(MVP, float4(input.Position, 1.0f));
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return albedoTexture.Sample(linearSampler, input.TexCoord);
}