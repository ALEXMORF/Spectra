#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), RootConstants(num32BitConstants=3, b0)"

#include "math.hlsl"
#include "edge_avoiding_functions.hlsl"

RWTexture2D<float2> LumMomentTex: register(u0);
RWTexture2D<float> VarianceTex: register(u1);

RWTexture2D<float4> InputTex: register(u2);
RWTexture2D<float4> GBufferTex: register(u3);

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
        float4 Geom = GBufferTex[ThreadId];
        float3 CenterN = Geom.xyz;
        float CenterDepth = Geom.w;
        
        float Mean = 0;
        float Mean2 = 0;
        float TotalContrib = 0;
        for (int dY = -3; dY <= 3; ++dY)
        {
            for (int dX = -3; dX <= 3; ++dX)
            {
                int2 Coord = int2(ThreadId) + int2(dX, dY);
                float4 TapGeom = GBufferTex[Coord];
                float3 Tap = InputTex[Coord].rgb;
                
                float3 TapN = TapGeom.xyz;
                float TapDepth = TapGeom.w;
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
        Variance *= 16.0; // boost to prevent underestimation
        VarianceTex[ThreadId] = Variance;
    }
}