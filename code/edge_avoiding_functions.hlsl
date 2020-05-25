#pragma once

float DepthWeight(float CenterDepth, float SampleDepth)
{
    return abs(1.0 - CenterDepth/SampleDepth) < 0.10? 1.0: 0.0;
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
