#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

struct VsIn
{
    float3 position : POSITION;
    float2 texCoords : TEXCOORD;
    int4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct VsOut
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
};

cbuffer MVP : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

cbuffer BoneData : register(b1)
{
    matrix boneMatrices[MAX_BONES];
};

VsOut main(VsIn vsIn)
{    
    float4 position = 0;
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vsIn.boneIndices[i] == -1)
            continue;
        
        if (vsIn.boneIndices[i] >= MAX_BONES)
        {
            position = float4(vsIn.position, 1.f);
            break;
        }
        
        float4 localPosition = mul(boneMatrices[vsIn.boneIndices[i]], float4(vsIn.position, 1.f));
        position += mul(localPosition, vsIn.boneWeights[i]);
    }
    
    VsOut vsOut;
    vsOut.position = mul(projection, mul(view, mul(model, position)));
    vsOut.texCoords = vsIn.texCoords;
    
    return vsOut;
}
