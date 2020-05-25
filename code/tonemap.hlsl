#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), RootConstants(num32BitConstants=1, b0)"

#include "math.hlsl"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> OutputTex: register(u1);
RWTexture2D<float4> NoisyInputTex: register(u2);

struct context
{
    int Threshold;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float3 Col = TaaInvTonemap(InputTex[ThreadId].rgb);
    
    if (ThreadId.x < Context.Threshold)
    {
        float3 NoisyCol = NoisyInputTex[ThreadId].rgb;
        Col = NoisyCol;
    }
    
    Col = Col/(1.0+Col);
    Col = sqrt(Col);
    
    OutputTex[ThreadId] = float4(Col, 1.0);
}