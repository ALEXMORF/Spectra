#define BIND(D, Id) Q.Dist = min(Q.Dist, D); if (Q.Dist == D) Q.MatId = Id;

float sdBox1(float3 P)
{
    P -= float3(1, -1.4, -0.5);
    P.xz = mul(P.xz, Rotate2(-0.3));
    float Dist = sdBox(P, float3(0.6, 0.6, 0.6));
    return Dist;
}

float sdBox2(float3 P)
{
    P -= float3(-0.7, -0.7, 0.4);
    P.xz = mul(P.xz, Rotate2(0.3));
    float Dist = sdBox(P, float3(0.6, 1.3, 0.6));
    return Dist;
}

point_query Map(float3 P, float Time)
{
    point_query Q;
    Q.Dist = 10e31;
    Q.MatId = -1;
    
    float Container = sdBox(P + float3(0, 0, 1.5), float3(2, 2, 3));
    Container = abs(Container) - 0.01;
    float Neg = dot(P-float3(0, 0, -1.5), float3(0, 0, -1));
    Container = max(Container, Neg);
    
    float Box1 = sdBox1(P);
    float Box2 = sdBox2(P);
    
    BIND(Container, 0);
    BIND(Box1, 1);
    BIND(Box2, 2);
    
    return Q;
}

material MapMaterial(int MatId, float3 P, float Time)
{
    material Mat;
    
    if (MatId == 0) // container
    {
        Mat.Albedo = 1.0;
        if (P.x <= -1.95) Mat.Albedo = float3(1.0, 0.1, 0.1);
        if (P.x >= 1.95) Mat.Albedo = float3(0.1, 1.0, 0.1);
        
        Mat.Emission = 0;
        
        bool IsInSquare = max(abs(P.x), abs(P.z)) < 0.6;
        if (P.y > 0.0 && IsInSquare) Mat.Emission = 10;
    }
    else if (MatId == 1)
    {
        Mat.Albedo = 1;
        Mat.Emission = 0;
    }
    else if (MatId == 2)
    {
        Mat.Albedo = 1;
        Mat.Emission = 0;
    }
    else // nothing
    {
        Mat.Albedo = 0;
        Mat.Emission = 0;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 0;
}

float Fog(float Depth)
{
    return 1.0;
}