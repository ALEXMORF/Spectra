#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), RootConstants(num32BitConstants=5, b0)"

#include "math.hlsl"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> OutputTex: register(u1);

RWTexture2D<float4> PositionTex: register(u2);
RWTexture2D<float4> NormalTex: register(u3);

RWTexture2D<float> VarianceTex: register(u4);
RWTexture2D<float> NextVarianceTex: register(u5);

struct context
{
    float3 CamP;
    int Depth;
    int Stride;
};

ConstantBuffer<context> Context: register(b0);

float DepthWeight(float CenterDepth, float SampleDepth)
{
    return abs(1.0 - CenterDepth/SampleDepth) < 0.01? 1.0: 0.0;
}

float NormalWeight(float3 CenterNormal, float3 SampleNormal)
{
    return pow(max(0.0, dot(CenterNormal, SampleNormal)), 32.0);
}

float LumWeight(float4 CenterCol, float4 SampleCol, float StdDeviation)
{
    float CenterLum = CalcLuminance(CenterCol.rgb);
    float SampleLum = CalcLuminance(SampleCol.rgb);
    
    float Epsilon = 0.000001;
    float Alpha = 4.0;
    return exp(-abs(CenterLum - SampleLum) / (Alpha*StdDeviation + Epsilon));
}

float FetchFilteredStdDeviation(int2 ThreadId)
{
    const float Gaussians[2] = {0.44198, 0.27901};
    
    float FilteredVariance = 0;
    float TotalContrib = 0;
    
    for (int dY = -1; dY <= 1; ++dY)
    {
        for (int dX = -1; dX <= 1; ++dX)
        {
            float W = Gaussians[abs(dX)]*Gaussians[abs(dY)];
            FilteredVariance += W * VarianceTex[ThreadId + int2(dX, dY)];
            TotalContrib += W;
        }
    }
    
    FilteredVariance /= TotalContrib;
    float FilteredStdDeviation = sqrt(FilteredVariance);
    return FilteredStdDeviation;
}

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float4 CenterCol = InputTex[ThreadId];
    float CenterDepth = length(Context.CamP - PositionTex[ThreadId].xyz);
    float3 CenterNormal = NormalTex[ThreadId].xyz;
    float StdDeviation = FetchFilteredStdDeviation(ThreadId);
    
    const float Kernel[3] = {3.0/8.0, 1.0/4.0, 1/16.0};
    
    float FilteredVariance = 0.0;
    float4 Filtered = 0.0;
    float TotalContrib = 0.0;
    for (int dY = -1; dY <= 1; ++dY)
    {
        for (int dX = -1; dX <= 1; ++dX)
        {
            int2 Coord = int2(ThreadId) + Context.Stride * int2(dX, dY);
            float4 Tap = InputTex[Coord];
            
            float W = Kernel[abs(dX)]*Kernel[abs(dY)];
            float TapDepth = length(Context.CamP - PositionTex[Coord].xyz);
            float3 TapNormal = NormalTex[Coord].xyz;
            W *= DepthWeight(CenterDepth, TapDepth);
            W *= NormalWeight(CenterNormal, TapNormal);
            W *= LumWeight(CenterCol, Tap, StdDeviation);
            
            Filtered += W * Tap;
            FilteredVariance += W*W * VarianceTex[Coord];
            TotalContrib += W;
        }
    }
    
    if (TotalContrib > 0.0)
    {
        Filtered /= TotalContrib;
        FilteredVariance /= TotalContrib*TotalContrib;
    }
    
    OutputTex[ThreadId] = Filtered;
    NextVarianceTex[ThreadId] = FilteredVariance;
}