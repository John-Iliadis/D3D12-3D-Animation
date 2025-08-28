struct VsOut
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
};

Texture2D baseColorTex : register(t0);
SamplerState texSampler : register(s0);

float4 main(VsOut input) : SV_Target
{
    return baseColorTex.Sample(texSampler, input.texCoords);
}
