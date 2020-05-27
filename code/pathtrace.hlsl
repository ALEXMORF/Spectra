#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), DescriptorTable(UAV(u5)), DescriptorTable(SRV(t0, numDescriptors=64)), RootConstants(num32BitConstants=14, b0)"

#include "constants.hlsl"
#include "scene.hlsl"
#include "random.hlsl"

RWTexture2D<float4> LightTex: register(u0);
RWTexture2D<float4> PositionTex: register(u1);
RWTexture2D<float4> NormalTex: register(u2);
RWTexture2D<float4> AlbedoTex: register(u3);
RWTexture2D<float4> EmissionTex: register(u4);
RWTexture2D<float4> RayDirTex: register(u5);

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

struct hit
{
    float T;
    int MatId;
};

float3 CalcGradient(float3 P)
{
    float2 E = float2(0, 0.0001);
    
    float3 Gradient;
    Gradient.x = Map(P + E.yxx, Context.Time).Dist - Map(P - E.yxx, Context.Time).Dist;
    Gradient.y = Map(P + E.xyx, Context.Time).Dist - Map(P - E.xyx, Context.Time).Dist;
    Gradient.z = Map(P + E.xxy, Context.Time).Dist - Map(P - E.xxy, Context.Time).Dist;
    
    return normalize(Gradient);
}

hit RayTrace(float3 Ro, float3 Rd)
{
    int MatId = -1;
    float T = 0.0;
    int Iter = 0;
    for (Iter = 0; Iter < 512 && T < T_MAX; ++Iter)
    {
        point_query Q = Map(Ro + T*Rd, Context.Time);
        if (Q.Dist < 0.001)
        {
            MatId = Q.MatId;
            break;
        }
        T += Q.Dist;
    }
    
    hit Hit;
    Hit.T = T;
    Hit.MatId = MatId;
    return Hit;
}

float3 Tonemap(float3 Col)
{
    return Col / (1.0 + Col);
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
    
    float3 Ro = Context.Ro;
    float3 At = Context.At;
    float3 CamZ = normalize(At - Ro);
    float3 CamX = normalize(cross(float3(0, 1, 0), CamZ));
    float3 CamY = cross(CamZ, CamX);
    float3 Rd = normalize(CamX * UV.x + CamY * UV.y + 1.7 * CamZ);
    RayDirTex[ThreadId] = float4(Rd, 0.0);
    
    int BounceCount = 4;
    
    float3 Radiance = 0.0;
    float3 Attenuation = 1.0;
    for (int Depth = 0; Depth < BounceCount; ++Depth)
    {
        hit Hit = RayTrace(Ro, Rd);
        float3 HitP = Ro + Hit.T*Rd;
        float3 HitN = CalcGradient(HitP);
        
        // record primary hits
        if (Depth == 0)
        {
            if (Hit.MatId != -1)
            {
                PositionTex[ThreadId] = float4(HitP, 0.0);
                NormalTex[ThreadId] = float4(HitN, 0.0);
            }
            else
            {
                PositionTex[ThreadId] = 10e31;
                NormalTex[ThreadId] = 0;
            }
        }
        
        if (Hit.MatId != -1)
        {
            material Mat = MapMaterial(Hit.MatId, HitP, Context.Time);
            
            //NOTE(chen): don't apply primary surface emission, only
            //            record indirect illumination for denoising,
            //            primary surface emission will be applied post-denoised
            if (Depth == 0)
            {
                EmissionTex[ThreadId] = float4(Mat.Emission, 1.0);
                AlbedoTex[ThreadId] = float4(Mat.Albedo, 1.0);
            }
            else
            {
                Radiance += Attenuation * Mat.Emission;
            }
            
            float3 Brdf = Mat.Albedo/Pi;
            if (Depth == 0)
            {
                //NOTE(chen): don't apply primary surface Brdf, only
                //            record indirect illumination for denoising,
                //            primary surface Brdf will be applied post-denoised
                //Brdf = Mat.Albedo/Pi;
                Brdf = 1.0;
            }
            
            Attenuation *= Brdf;
            
            Ro = HitP + 0.01*HitN;
            float2 R = BlueNoiseTexs[Context.FrameIndex & 63][ThreadId & 63].rg;
            Rd = SampleHemisphereCosineWeighted(HitN, R);
        }
        else
        {
            Radiance += Attenuation * Env(Rd, Context.Time);
            break;
        }
    }
    
    LightTex[ThreadId] = float4(Radiance, 1.0);
}
