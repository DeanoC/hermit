struct FSInput {
    float4 Position : SV_POSITION;
    float4 Colour   : COLOR;
};

float4 FS_main(FSInput input) : SV_Target
{
    return input.Colour;
}
