#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), DescriptorTable(UAV(u6)), DescriptorTable(UAV(u7)), DescriptorTable(UAV(u8)), DescriptorTable(UAV(u9)), RootConstants(num32BitConstants=10, b0)"

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

RWTexture2D<float2> PrevPixelIdTex: register(u9);

struct context
{
    int FrameIndex;
    float3 CamP;
    float4 CamInvQuat;
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
    
    float CenterLum = CalcLuminance(InputTex[ThreadId].rgb);
    float2 LumMoments = float2(CenterLum, CenterLum * CenterLum);
    
    if (Context.FrameIndex != 0 &&
        all(PrevPixelId >= 0.0 && PrevPixelId <= float2(Context.Width-1, Context.Height-1)))
    {
        float2 Interp = frac(PrevPixelId);
        int2 PrevPixelCoord = floor(PrevPixelId);
        
        float MinSampleCount = 10e31;
        float2 LumMomentHist = 0;
        float3 FilteredHist = 0;
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
                    dot(CurrN, PrevN) >= 0.9)
                {
                    float2 Bilinear = lerp(1.0-Interp, Interp, float2(dX, dY));
                    float W = Bilinear.x * Bilinear.y;
                    FilteredHist += W * LightHistTex[TapCoord].rgb;
                    LumMomentHist += W *LumMomentHistTex[TapCoord];
                    TotalContrib += W;
                    
                    float TapSampleCount = LightHistTex[TapCoord].a;
                    MinSampleCount = min(MinSampleCount, TapSampleCount);
                }
            }
        }
        
        if (TotalContrib > 0.0) 
        {
            FilteredHist /= TotalContrib;
            LumMomentHist /= TotalContrib;
            
            float Alpha = 0.1;
            
            float3 FilteredCol = lerp(FilteredHist.rgb, InputTex[ThreadId].rgb, Alpha);
            
            float EffectiveSampleCount = MinSampleCount + 1;
            FilteredTex[ThreadId] = float4(FilteredCol, EffectiveSampleCount);
            
            LumMomentTex[ThreadId] = lerp(LumMomentHist, LumMoments, Alpha);
        }
        else
        {
            float SampleCount = 1;
            FilteredTex[ThreadId] = float4(InputTex[ThreadId].rgb, SampleCount);
            LumMomentTex[ThreadId] = LumMoments;
        }
    }
    else
    {
        float SampleCount = 1;
        FilteredTex[ThreadId] = float4(InputTex[ThreadId].rgb, SampleCount);
        LumMomentTex[ThreadId] = LumMoments;
    }
}