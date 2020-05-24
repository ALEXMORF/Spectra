#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), RootConstants(num32BitConstants=3, b0)"

#include "math.hlsl"
#include "edge_avoiding_functions.hlsl"

RWTexture2D<float2> LumMomentTex: register(u0);
RWTexture2D<float> VarianceTex: register(u1);

RWTexture2D<float4> InputTex: register(u2);
RWTexture2D<float4> PositionTex: register(u3);
RWTexture2D<float4> NormalTex: register(u4);

struct context
{
    float3 CamP;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float SampleCount = InputTex[ThreadId].a;
    
    if (SampleCount > 4) // enough temporal data to estimate variance
    {
        float2 Moments = LumMomentTex[ThreadId];
        float Mean = Moments.x;
        float Mean2 = Moments.y;
        float Variance = abs(Mean2 - Mean*Mean);
        VarianceTex[ThreadId] = Variance;
    }
    else // spatial variance estimation fallback
    {
        float3 CenterN = NormalTex[ThreadId].xyz;
        float CenterDepth = length(Context.CamP - PositionTex[ThreadId].xyz);
        
        float Mean = 0;
        float Mean2 = 0;
        float TotalContrib = 0;
        for (int dY = -3; dY <= 3; ++dY)
        {
            for (int dX = -3; dX <= 3; ++dX)
            {
                int2 Coord = int2(ThreadId) + int2(dX, dY);
                float3 TapN = NormalTex[Coord].xyz;
                float TapDepth = length(Context.CamP - PositionTex[Coord].xyz);
                float3 Tap = InputTex[Coord].rgb;
                float TapLum = CalcLuminance(Tap);
                
                float W = DepthWeight(CenterDepth, TapDepth);
                W *= NormalWeight(CenterN, TapN);
                
                Mean += W * TapLum;
                Mean2 += W * TapLum*TapLum;
                TotalContrib += W;
            }
        }
        
        if (TotalContrib > 0.0) 
        {
            Mean /= TotalContrib;
            Mean2 /= TotalContrib;
        }
        
        float Variance = abs(Mean2 - Mean*Mean);
        Variance *= 8.0; // boost to prevent underestimation
        VarianceTex[ThreadId] = Variance;
    }
}