#define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"

struct vs_in
{
    float2 P: POS;
    float2 UV: TEXCOORD;
};

struct vs_out
{
    float4 SV_P: SV_Position;
    float2 UV: TEXCOORD;
};

[RootSignature(RS)]
vs_out VS(vs_in In)
{
    vs_out Out;
    
    Out.SV_P = float4(In.P, 0.0, 1.0);
    Out.UV = In.UV;
    
    return Out;
}

float4 PS(vs_out In): SV_Target
{
    return 1.0;
}