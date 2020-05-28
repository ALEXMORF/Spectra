#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), RootConstants(num32BitConstants=6, b0)"

#include "math.hlsl"

RWTexture2D<float4> PositionTex: register(u0);
RWTexture2D<float4> NormalTex: register(u1);
RWTexture2D<float4> PositionHistTex: register(u2);
RWTexture2D<float4> NormalHistTex: register(u3);
RWTexture2D<float2> PrevPixelIdTex: register(u4);
RWTexture2D<uint> DisocclusionTex: register(u5);

struct context
{
    int FrameIndex;
    float3 CamP;
    int Width;
    int Height;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float3 P = PositionTex[ThreadId].xyz;
    float3 CurrN = NormalTex[ThreadId].xyz;
    float2 PrevPixelId = PrevPixelIdTex[ThreadId];
    
    uint IsDisoccluded = 1;
    if (all(PrevPixelId >= 0.0 && PrevPixelId <= float2(Context.Width-1, Context.Height-1)))
    {
        float2 Interp = frac(PrevPixelId);
        int2 PrevPixelCoord = floor(PrevPixelId);
        
        float TotalContrib = 0;
        for (int dY = 0; dY <= 1; ++dY)
        {
            for (int dX = 0; dX <= 1; ++dX)
            {
                int2 TapCoord = PrevPixelCoord + int2(dX, dY);
                float3 PrevN = NormalHistTex[TapCoord].xyz;
                float3 PrevP = PositionHistTex[TapCoord].xyz;
                float PrevDepth = length(Context.CamP - PrevP);
                float CurrDepth = length(Context.CamP - P);
                if (abs(1.0 - CurrDepth/PrevDepth) < 0.1 &&
                    dot(CurrN, PrevN) >= 0.99)
                {
                    float2 Bilinear = lerp(1.0-Interp, Interp, float2(dX, dY));
                    float W = Bilinear.x * Bilinear.y;
                    TotalContrib += W;
                }
            }
        }
        
        if (TotalContrib > 0.0) 
        {
            IsDisoccluded = 0;
        }
    }
    
    DisocclusionTex[ThreadId] = IsDisoccluded;
}
