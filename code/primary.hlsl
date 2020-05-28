#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), RootConstants(num32BitConstants=14, b0)"

#include "constants.hlsl"
#include "scene.hlsl"
#include "raymarch.hlsl"

RWTexture2D<float4> PositionTex: register(u0);
RWTexture2D<float4> NormalTex: register(u1);
RWTexture2D<float4> AlbedoTex: register(u2);
RWTexture2D<float4> EmissionTex: register(u3);
RWTexture2D<float4> RayDirTex: register(u4);

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

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float2 UV = (float2(ThreadId) + 0.5 + Context.PixelOffset) / float2(Context.Width, Context.Height);
    UV = 2.0 * UV - 1.0;
    UV.y = -UV.y;
    UV.x *= float(Context.Width)/float(Context.Height);
    
    float3 Ro = Context.Ro;
    float3 At = Context.At;
    float3 CamZ = normalize(At - Ro);
    float3 CamX = normalize(cross(float3(0, 1, 0), CamZ));
    float3 CamY = cross(CamZ, CamX);
    float3 Rd = normalize(CamX * UV.x + CamY * UV.y + 1.7 * CamZ);
    RayDirTex[ThreadId] = float4(Rd, 0.0);
    
    hit Hit = RayMarch(Ro, Rd, Context.Time);
    float3 HitP = Ro + Hit.T*Rd;
    float3 HitN = CalcGradient(HitP, Context.Time);
    
    if (Hit.MatId != -1)
    {
        PositionTex[ThreadId] = float4(HitP, 0.0);
        NormalTex[ThreadId] = float4(HitN, 0.0);
        
        material Mat = MapMaterial(Hit.MatId, HitP, Context.Time);
        EmissionTex[ThreadId] = float4(Mat.Emission, 1.0);
        AlbedoTex[ThreadId] = float4(Mat.Albedo, 1.0);
    }
    else
    {
        PositionTex[ThreadId] = 10e31;
        NormalTex[ThreadId] = 0;
    }
}
