cbuffer View : register(b0, space1)
{
    float4x4 worldToViewMatrix;
    float4x4 viewToNDCMatrix;
    float4x4 worldToNDCMatrix;
};

Texture2D foodTexture : register(t0, space1);

struct VSInput
{
    float4 Position : POSITION;
    float2 uv : TEX_COORD0;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float4 Colour   : COLOR;
};

VSOutput VS_main(VSInput input)
{
    VSOutput result;

    float fi = foodTexture.SampleLevel(pointSampler, input.uv, 0);
    float4 pos = input.Position;
    pos.y += fi;

    result.Position = mul(worldToNDCMatrix, pos);
    result.Colour = float4(fi, 0.3, 0.3, 1.0);
    return result;
}