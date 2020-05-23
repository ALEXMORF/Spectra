#define RS "DescriptorTable(UAV(u0)), RootConstants(num32BitConstants=2, b0)"

RWTexture2D<float4> OutputTex: register(u0);

struct context
{
    int Width;
    int Height;
};

ConstantBuffer<context> Context: register(b0);

struct query
{
    float Dist;
    int Id;
};

query Map(float3 P)
{
    query Q;
    
    float Floor = P.y;
    float Sphere = length(P - float3(0, 1, 0)) - 1.0;
    Q.Dist = min(Floor, Sphere);
    
    if (Q.Dist == Floor)
    {
        Q.Id = 0;
    }
    else if (Q.Dist == Sphere)
    {
        Q.Id = 1;
    }
    else
    {
        Q.Id = -1;
    }
    
    return Q;
}

float3 MapAlbedo(int ObjId, float3 P)
{
    if (ObjId == 0)
    {
        return float3(0.5, 0.5, 0.3);
    }
    else // objId == 1
    {
        return float3(1.0, 0.5, 0.5);
    }
}

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float2 UV = float2(ThreadId) / float2(Context.Width, Context.Height);
    UV = 2.0 * UV - 1.0;
    UV.y = -UV.y;
    UV.x *= float(Context.Width)/float(Context.Height);
    
    float3 Ro = float3(0, 1.2, -4);
    float3 At = float3(0, 1, 0);
    float3 CamZ = normalize(At - Ro);
    float3 CamX = normalize(cross(float3(0, 1, 0), CamZ));
    float3 CamY = cross(CamZ, CamX);
    float3 Rd = normalize(CamX * UV.x + CamY * UV.y + 1.7 * CamZ);
    
    float TMax = 100.0;
    
    int ObjId = -1;
    float T = 0.0;
    int Iter = 0;
    for (Iter = 0; Iter < 256 && T < TMax; ++Iter)
    {
        query Q = Map(Ro + T*Rd);
        if (Q.Dist < 0.01)
        {
            ObjId = Q.Id;
            break;
        }
        T += Q.Dist;
    }
    
    float3 Col = 0.0;
    if (ObjId != -1)
    {
        float3 HitP = Ro + T*Rd;
        float3 Albedo = MapAlbedo(ObjId, HitP);
        Col = Albedo;
    }
    
    OutputTex[ThreadId] = float4(Col, 1.0);
}
