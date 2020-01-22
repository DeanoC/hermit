cbuffer View : register(b0, space1)
{
    float4x4 worldToViewMatrix;
    float4x4 viewToNDCMatrix;
    float4x4 worldToNDCMatrix;
};

cbuffer LocalToWorld : register(b1, space3)
{
    float4x4 localToWorldMatrix;
    float4x4 localToWorldMatrixTranspose;
};

struct VSInput
{
    float4 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Colour   : COLOR;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float4 Colour   : COLOR;
};

VSOutput VS_main(VSInput input)
{
    VSOutput result;

    result.Position = mul(localToWorldMatrix, input.Position);
    float4 worldNormal = mul(localToWorldMatrixTranspose, float4(input.Normal,0));
    result.Position = mul(worldToNDCMatrix, result.Position);
    float l = dot(worldNormal, normalize(float3(-1,-1,1)));
    result.Colour = (max(l,0) + 0.1f) * input.Colour;
    return result;
}