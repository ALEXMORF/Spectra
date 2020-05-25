#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), RootConstants(num32BitConstants=8, b0)"

#include "math.hlsl"

RWTexture2D<float4> PositionTex: register(u0);
RWTexture2D<float4> NormalTex: register(u1);
RWTexture2D<float4> GBufferTex: register(u2);

struct context
{
    float3 CamP;
    uint Pad1;
    
    float4 CamInvQuat;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float3 P = PositionTex[ThreadId].xyz;
    float3 ViewP = qrot(P - Context.CamP, Context.CamInvQuat);
    
    float Depth = ViewP.z;
    float3 Normal = NormalTex[ThreadId].xyz;
    GBufferTex[ThreadId] = float4(Normal, Depth);
}