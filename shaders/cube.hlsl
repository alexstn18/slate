cbuffer ModelViewProjectionCB : register(b0)
{
    matrix MVP;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float3 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    result.position = mul(MVP, float4(position, 1.0));
    result.color = color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}