#define RS "DescriptorTable(UAV(u0)), RootConstants(num32BitConstants=3, b0)"

RWTexture2D<float4> OutputTex: register(u0);

struct context
{
    int Width;
    int Height;
    int FrameIndex;
};

ConstantBuffer<context> Context: register(b0);

#define Pi 3.1415926

static float2 gSeed;

float2 Rand2() {
    gSeed += float2(-1,1);
    return float2(frac(sin(dot(gSeed.xy ,float2(12.9898,78.233))) * 43758.5453),
                  frac(cos(dot(gSeed.xy ,float2(4.898,7.23))) * 23421.631));
};

float3 SampleHemisphereCosineWeighted(float3 N)
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
    
    float2 R = Rand2();
    float Radius = sqrt(R.x);
    float Angle = R.y * 2.0 * Pi;
    
    float X = Radius * cos(Angle);
    float Y = Radius * sin(Angle);
    float Z = sqrt(1.0 - Radius*Radius);
    
    return X*XAxis + Y*YAxis + Z*N;
}

struct query
{
    float Dist;
    int MatId;
};

struct material
{
    float3 Albedo;
    float3 Emission;
};

struct hit
{
    float T;
    int MatId;
};

float2x2 Rotate2D(float theta)
{
    return float2x2(cos(theta), -sin(theta), sin(theta), cos(theta));
}

float Sculpture(float3 p)
{
    float amp = 1.0;
    float freq = 2.0;
    for (int i = 0; i < 3; ++i)
    {
        float s = i % 2 == 0? 1.0: -1.0;
        p.xz = mul(p.xz, Rotate2D(s * (1.0 + 3.0*float(i))));
        p += amp * sin(freq * p.zxy);
        amp *= 0.5;
        freq *= 2.1;
    }
    return 0.1 * (length(p) - 1.0);
}

query Map(float3 P)
{
    query Q;
    
    float Floor = P.y;
    float Sphere = Sculpture(P - float3(0, 1, 0));
    float Light = length(P - float3(0, 3.5, 0)) - 0.8;
    Q.Dist = min(min(Floor, Sphere), Light);
    
    if (Q.Dist == Floor)
    {
        Q.MatId = 0;
    }
    else if (Q.Dist == Sphere)
    {
        Q.MatId = 1;
    }
    else if (Q.Dist == Light)
    {
        Q.MatId = 2;
    }
    else
    {
        Q.MatId = -1;
    }
    
    return Q;
}

material MapMaterial(int ObjId, float3 P)
{
    material Mat;
    
    if (ObjId == 0) // floor
    {
        Mat.Albedo = float3(0.5, 0.5, 0.3);
        Mat.Emission = 0.0;
    }
    else if (ObjId == 1) // sphere
    {
        Mat.Albedo = 1.0;
        Mat.Emission = 0.0;
    }
    else if (ObjId == 2) // light
    {
        Mat.Albedo = 1.0;
        Mat.Emission = 10.0;
    }
    else // invalid id
    {
        // give it a disgusting purple
        Mat.Albedo = float3(1.0, 0.0, 1.0);
        Mat.Emission = 0.0;
    }
    
    return Mat;
}

float3 CalcGradient(float3 P)
{
    float2 E = float2(0, 0.0001);
    
    float3 Gradient;
    Gradient.x = Map(P + E.yxx).Dist - Map(P).Dist;
    Gradient.y = Map(P + E.xyx).Dist - Map(P).Dist;
    Gradient.z = Map(P + E.xxy).Dist - Map(P).Dist;
    
    return normalize(Gradient);
}

#define T_MAX 100.0

hit RayTrace(float3 Ro, float3 Rd)
{
    int MatId = -1;
    float T = 0.0;
    int Iter = 0;
    for (Iter = 0; Iter < 256 && T < T_MAX; ++Iter)
    {
        query Q = Map(Ro + T*Rd);
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
    float2 UV = float2(ThreadId) / float2(Context.Width, Context.Height);
    UV = 2.0 * UV - 1.0;
    UV.y = -UV.y;
    UV.x *= float(Context.Width)/float(Context.Height);
    
    gSeed = UV * (float(Context.FrameIndex)+1.0);
    
    float3 Ro = float3(0, 1.2, -4);
    float3 At = float3(0, 1, 0);
    float3 CamZ = normalize(At - Ro);
    float3 CamX = normalize(cross(float3(0, 1, 0), CamZ));
    float3 CamY = cross(CamZ, CamX);
    float3 Rd = normalize(CamX * UV.x + CamY * UV.y + 1.7 * CamZ);
    
    int BounceCount = 4;
    
    float3 Radiance = 0.0;
    float3 Attenuation = 1.0;
    for (int Depth = 0; Depth < BounceCount; ++Depth)
    {
        hit Hit = RayTrace(Ro, Rd);
        if (Hit.MatId != -1)
        {
            float3 HitP = Ro + Hit.T*Rd;
            float3 HitN = CalcGradient(HitP);
            material Mat = MapMaterial(Hit.MatId, HitP);
            
            Radiance += Attenuation * Mat.Emission;
            float3 BRDF = Mat.Albedo/Pi * clamp(dot(HitN, -Rd), 0.0, 1.0);
            Attenuation *= BRDF;
            
            Ro = HitP + 0.01*HitN;
            Rd = SampleHemisphereCosineWeighted(HitN);
        }
        else
        {
            break;
        }
    }
    
    float3 Col = Radiance;
    
    if (Context.FrameIndex != 0)
    {
        float3 HistCol = OutputTex[ThreadId].rgb;
        OutputTex[ThreadId] = float4(lerp(HistCol, Col, 0.2), 1.0);
    }
    else
    {
        OutputTex[ThreadId] = float4(Col, 1.0);
    }
}
