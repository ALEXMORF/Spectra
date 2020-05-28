point_query Map(float3 P, float Time)
{
    point_query Q;
    
    float Container = sdBox(P - float3(0, 0, 1.5), float3(2, 2, 3));
    Container = abs(Container) - 0.01;
    float Neg = dot(P-float3(0, 0, 1.5), float3(0, 0, 1));
    Container = max(Container, Neg);
    
    float Box1 = sdBox(P - float3(-1, -1, 0), float3(0.5, 1, 0.5));
    
    Q.Dist = min(Container, Box1);
    
    if (Q.Dist == Container)
    {
        Q.MatId = 0;
    }
    else if (Q.Dist == Box1)
    {
        Q.MatId = 1;
    }
    else 
    {
        Q.MatId = -1;
    }
    
    return Q;
}

material MapMaterial(int MatId, float3 P, float Time)
{
    material Mat;
    
    if (MatId == 0) // container
    {
        Mat.Albedo = 1.0;
        if (P.x <= -1.95) Mat.Albedo = float3(0.1, 0.1, 1);
        if (P.x >= 1.95) Mat.Albedo = float3(1.0, 0.1, 0.1);
        
        Mat.Emission = 0;
        
        if (P.y > 0.0 && length(P.xz) < 0.5) Mat.Emission = 200;
    }
    else if (MatId == 1)
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
