#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), DescriptorTable(UAV(u6)), DescriptorTable(SRV(t0, numDescriptors=64)), RootConstants(num32BitConstants=14, b0)"

#include "constants.hlsl"
#include "scene.hlsl"
#include "raymarch.hlsl"
#include "random.hlsl"

RWTexture2D<float4> LightTex: register(u0);
RWTexture2D<float4> PositionTex: register(u1);
RWTexture2D<float4> NormalTex: register(u2);
RWTexture2D<float4> AlbedoTex: register(u3);
RWTexture2D<float4> EmissionTex: register(u4);
RWTexture2D<float4> RayDirTex: register(u5);
RWTexture2D<uint> DisocclusionTex: register(u6);

Texture2D<float4> BlueNoiseTexs[64]: register(t0);

struct context
{
    int Width;
    int Height;
    int FrameIndex;
    uint Pad1;
    
    float3 Ro;
    uint Pad2;
    float3 At;
    float Time;
    
    float2 PixelOffset;
};

ConstantBuffer<context> Context: register(b0);

float3 SampleHemisphereCosineWeighted(float3 N, float2 R)
{
    float3 XAxis;
    if (abs(N.y) > 0.99)
    {
        XAxis = normalize(cross(N, float3(0, 0, 1)));
    }
    else
    {
        XAxis = normalize(cross(float3(0, 1, 0), N));
    }
    
    float3 YAxis = cross(N, XAxis);
    
    float Radius = sqrt(R.x);
    float Angle = R.y * 2.0 * Pi;
    
    float X = Radius * cos(Angle);
    float Y = Radius * sin(Angle);
    float Z = sqrt(1.0 - Radius*Radius);
    
    return X*XAxis + Y*YAxis + Z*N;
}

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float2 UV = (float2(ThreadId) + 0.5 + Context.PixelOffset) / float2(Context.Width, Context.Height);
    UV = 2.0 * UV - 1.0;
    UV.y = -UV.y;
    UV.x *= float(Context.Width)/float(Context.Height);
    
    gSeed = UV * (float(Context.FrameIndex)+1.0);
    
    bool HasNoHistory = DisocclusionTex[ThreadId] == 1;
    
    float3 HitP = PositionTex[ThreadId].xyz;
    float3 HitN = NormalTex[ThreadId].xyz;
    float3 Radiance = 0;
    
    if (length(HitP) < 10e30)
    {
        int SampleCount = 1;
        if (HasNoHistory) SampleCount *= 8;
        float SampleWeight = 1.0 / float(SampleCount);
        
        float3 SampleAvg = 0;
        
        for (int SampleI = 0; SampleI < SampleCount; ++SampleI)
        {
            int BounceCount = 3;
            
            float3 Sample = 0;
            float3 Attenuation = 1.0;
            for (int Depth = 0; Depth < BounceCount; ++Depth)
            {
                float3 Ro = HitP + 0.01*HitN;
                float2 R = Rand2();
                float3 Rd = SampleHemisphereCosineWeighted(HitN, R);
                
                hit Hit = RayMarch(Ro, Rd, Context.Time);
                if (Hit.MatId != -1)
                {
                    HitP = Ro + Hit.T*Rd;
                    HitN = CalcGradient(HitP, Context.Time);
                    
                    material Mat = MapMaterial(Hit.MatId, HitP, Context.Time);
                    
                    Sample += Attenuation * Mat.Emission;
                    float3 Brdf = Mat.Albedo;
                    Attenuation *= Brdf;
                }
                else
                {
                    Sample += Attenuation * Env(Rd, Context.Time);
                    break;
                }
            }
            
            SampleAvg += SampleWeight * Sample;
            
            // reload primary surface for next round of sampling
            HitP = PositionTex[ThreadId].xyz;
            HitN = NormalTex[ThreadId].xyz;
        }
        
        Radiance = SampleAvg;
    }
    
    LightTex[ThreadId] = float4(Radiance, 1.0);
}
