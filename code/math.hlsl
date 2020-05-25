#pragma once

#include "quaternion.hlsl"

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return max(dot(color, float3(0.299f, 0.587f, 0.114f)), 0.0001f);
}

float3 CalcViewP(float3 P, float3 CamP, float4 CamInvQuat)
{
    return qrot(P - CamP, CamInvQuat);
}
