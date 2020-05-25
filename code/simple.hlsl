point_query Map(float3 P, float Time)
{
    point_query Q;
    
    float Floor = P.y;
    float Sphere = length(P - float3(0, 1, 0)) - 1.0;
    Q.Dist = min(Floor, Sphere);
    
    if (Q.Dist == Floor)
    {
        Q.MatId = 0;
    }
    else if (Q.Dist == Sphere)
    {
        Q.MatId = 1;
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
    
    if (ObjId == 1) // Sphere
    {
        Mat.Albedo = 1.0;
        Mat.Emission = 0.0;
        
        if (abs(P.x - 0.0) < 0.1) Mat.Emission = P;
    }
    else // invalid id (default)
    {
        Mat.Albedo = 1.0;
        Mat.Emission = 0.0;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 0.01*float3(0.3, 0.4, 0.5);
}
