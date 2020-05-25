#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), RootConstants(num32BitConstants=11, b0)"

#include "math.hlsl"
#include "random.hlsl"

RWTexture2D<float4> PositionTex: register(u0);
RWTexture2D<float2> PrevPixelIdTex: register(u1);

struct context
{
    float3 PrevCamP;
    uint Pad;
    
    float4 PrevCamInvQuat;
    
    int Width;
    int Height;
    int FrameIndex;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float3 P = PositionTex[ThreadId].xyz;
    float3 PrevViewP = CalcViewP(P, Context.PrevCamP, Context.PrevCamInvQuat);
    float2 PrevUV = PrevViewP.xy / PrevViewP.z * 1.7;
    PrevUV.y = -PrevUV.y;
    PrevUV.x /= float(Context.Width) / float(Context.Height);
    float2 PrevPixelId = (0.5 * PrevUV + 0.5) * float2(Context.Width,
                                                       Context.Height);
    PrevPixelId -= 0.5;
    
    // unjitter
    gSeed = float(Context.FrameIndex)+1.0;
    float2 Jitter = Rand2() - 0.5;
    PrevPixelId -= Jitter;
    
    PrevPixelIdTex[ThreadId] = PrevPixelId;
}