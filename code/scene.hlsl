struct point_query
{
    float Dist;
    int MatId;
};

struct material
{
    float3 Albedo;
    float3 Emission;
};

#if 1

#include "simple.hlsl"

#else

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

point_query Map(float3 P, float Time)
{
    point_query Q;
    
    float Floor = P.y;
    float Sculpt = Sculpture(P - float3(0, 1, 0));
    float Light = -(P.x - 10.0);
    Q.Dist = min(min(Floor, Sculpt), Light);
    
    if (Q.Dist == Floor)
    {
        Q.MatId = 0;
    }
    else if (Q.Dist == Sculpt)
    {
        Q.MatId = 1;
    }
    else if (Q.Dist = Light)
    {
        Q.MatId = 2;
    }
    else
    {
        Q.MatId = -1;
    }
    
    return Q;
}

material MapMaterial(int ObjId, float3 P, float Time)
{
    material Mat;
    
    if (ObjId == 0) // floor
    {
        Mat.Albedo = float3(0.5, 0.5, 0.3);
        Mat.Emission = 0.0;
    }
    else if (ObjId == 1) // Sculpt
    {
        Mat.Albedo = 1.0;
        Mat.Emission = 0.0;
    }
    else if (ObjId == 2) // Light
    {
        Mat.Albedo = 1.0;
        float K = 0.0;
        if (P.y > 5.0 && P.y < 10.0) K = 3.0;
        Mat.Emission = K * float3(frac(0.001*P.y), 1.0, frac(0.001*P.z));
    }
    else // invalid id
    {
        // give it a disgusting purple
        Mat.Albedo = float3(1.0, 0.0, 1.0);
        Mat.Emission = 0.0;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 0.0*float3(0.3, 0.4, 0.5);
}

#endif