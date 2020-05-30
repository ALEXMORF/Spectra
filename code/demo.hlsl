#define BIND(D, Id) Q.Dist = min(Q.Dist, D); if (Q.Dist == D) Q.MatId = Id;

point_query Map(float3 P, float Time)
{
    point_query Q;
    Q.Dist = 10e31;
    Q.MatId = -1;
    
    float Floor = P.y + 1.0;
    float Sphere = length(P) - 1.0;
    float Sphere2 = length(P - float3(2, 0, 0)) - 1.0;
    
    BIND(Floor, 0);
    BIND(Sphere, 1);
    BIND(Sphere2, 2);
    
    return Q;
}

material MapMaterial(int MatId, float3 P, float Time)
{
    material Mat;
    Mat.Albedo = 0.9;
    Mat.Emission = 0;
    
    if (MatId == 0) // floor
    {
        Mat.Albedo = 0.5;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 1.0 * float3(0.3, 0.4, 0.5);
}
