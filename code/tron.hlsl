#define BIND(D, Id) Q.Dist = min(Q.Dist, D); if (Q.Dist == D) Q.MatId = Id;

point_query Map(float3 P, float Time)
{
    point_query Q;
    Q.Dist = 10e31;
    Q.MatId = -1;
    
    P.z += 1.0;
    P = fmod(P-2.5, 5.0)+2.5;
    
    float Floor = P.y + 1.0;
    float Sphere = length(P) - 1.0;
    
    BIND(Floor, 0);
    BIND(Sphere, 1);
    
    return Q;
}

material MapMaterial(int MatId, float3 P, float Time)
{
    material Mat;
    Mat.Albedo = 0.9;
    Mat.Emission = 0;
    
    if (MatId == 0)
    {
        Mat.Albedo = 0.8;
    }
    else if (MatId == 1)
    {
        Mat.Albedo = 1;
        
        P.z += 1.0;
        P = fmod(P-2.5, 5.0)+2.5;
        if (abs(P.x) < 0.1) Mat.Emission = P+1.0;
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