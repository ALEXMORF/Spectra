#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), DescriptorTable(UAV(u6)), DescriptorTable(UAV(u7)), DescriptorTable(UAV(u8)), RootConstants(num32BitConstants=14, b0)"

#include "math.hlsl"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> LightHistTex: register(u1);
RWTexture2D<float4> FilteredTex: register(u2);

RWTexture2D<float4> PositionTex: register(u3);
RWTexture2D<float4> NormalTex: register(u4);

RWTexture2D<float4> PositionHistTex: register(u5);
RWTexture2D<float4> NormalHistTex: register(u6);

RWTexture2D<float2> LumMomentHistTex: register(u7);
RWTexture2D<float2> LumMomentTex: register(u8);

struct context
{
    int FrameIndex;
    float3 PrevCamP;
    
    float4 PrevCamInvQuat;
    
    float3 CamP;
    uint Pad;
    
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
    float3 ClipP = qrot(P - Context.PrevCamP, Context.PrevCamInvQuat);
    float2 UV = ClipP.xy / ClipP.z * 1.7;
    UV.y = -UV.y;
    UV.x /= float(Context.Width) / float(Context.Height);
    float2 HistPixelId = (0.5 * UV + 0.5) * float2(Context.Width,
                                                   Context.Height);
    
    float CenterLum = CalcLuminance(InputTex[ThreadId].rgb);
    float2 LumMoments = float2(CenterLum, CenterLum * CenterLum);
    
    if (Context.FrameIndex != 0 || 
        any(HistPixelId < 0.0 || HistPixelId > float2(Context.Width-1, Context.Height-1)))
    {
        float2 Interp = frac(HistPixelId);
        int2 PixelCoord = floor(HistPixelId);
        
        float2 LumMomentHist = 0;
        float4 FilteredHist = 0;
        float TotalContrib = 0;
        for (int dY = 0; dY <= 1; ++dY)
        {
            for (int dX = 0; dX <= 1; ++dX)
            {
                int2 TapCoord = PixelCoord + int2(dX, dY);
                float3 PrevN = NormalHistTex[TapCoord].xyz;
                float3 PrevP = PositionHistTex[TapCoord].xyz;
                float PrevDepth = length(Context.CamP - PrevP);
                float CurrDepth = length(Context.CamP - P);
                if (abs(1.0 - CurrDepth/PrevDepth) < 0.1 &&
                    dot(CurrN, PrevN) >= 0.9)
                {
                    float2 Bilinear = lerp(1.0-Interp, Interp, float2(dX, dY));
                    float W = Bilinear.x * Bilinear.y;
                    FilteredHist += W *LightHistTex[TapCoord];
                    LumMomentHist += W *LumMomentHistTex[TapCoord];
                    TotalContrib += W;
                }
            }
        }
        
        if (TotalContrib > 0.0) 
        {
            FilteredHist /= TotalContrib;
            LumMomentHist /= TotalContrib;
            
            float Alpha = 0.1;
            FilteredTex[ThreadId] = lerp(FilteredHist, InputTex[ThreadId], Alpha);
            LumMomentTex[ThreadId] = lerp(LumMomentHist, LumMoments, Alpha);
        }
        else
        {
            FilteredTex[ThreadId] = InputTex[ThreadId];
            LumMomentTex[ThreadId] = LumMoments;
        }
    }
    else
    {
        FilteredTex[ThreadId] = InputTex[ThreadId];
        LumMomentTex[ThreadId] = LumMoments;
    }
}