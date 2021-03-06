#define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), DescriptorTable(SRV(t0)), StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR)"

Texture2D<float> FontAtlasTex: register(t0);
SamplerState LinearSampler: register(s0);

struct vs_in
{
    float2 P: POS;
    float2 UV: TEXCOORD;
    int Type: TYPE;
};

struct vs_out
{
    float4 SV_P: SV_Position;
    float2 UV: TEXCOORD;
    int Type: TYPE;
};

[RootSignature(RS)]
vs_out VS(vs_in In)
{
    vs_out Out;
    
    Out.SV_P = float4(In.P, 0.0, 1.0);
    Out.UV = In.UV;
    Out.Type = In.Type;
    
    return Out;
}

float4 PS(vs_out In): SV_Target
{
    if (In.Type == 0) // text
    {
        float2 ST = In.UV;
        float Alpha = FontAtlasTex.Sample(LinearSampler, ST);
        return float4(1.0, 1.0, 1.0, Alpha);
    }
    else // background
    {
        return float4(0, 0, 0, 0.5);
    }
}