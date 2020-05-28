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

float sdBox(float3 P, float3 Radius)
{
    float3 Q = abs(P) - Radius;
    return length(max(Q, 0)) + min(max(max(Q.x, Q.y), Q.z), 0.0);
}

point_query Map(float3 P, float Time)
{
    point_query Q;
    
    float Room = sdBox(P - float3(0, 5, 0), 5);;
    Room = abs(Room)-0.01;
    float Neg = -sdBox(P - float3(5.0, 10.0, 0), float3(1, 1, 4));
    Room = max(Room, Neg);
    
    float Shape = sdBox(P - float3(0,1,0), 1);
    Shape = abs(Shape) - 0.03;
    Shape = max(Shape, dot(P, float3(1,1,-1))-2.5);
    Shape = 0.5 * Shape;
    
    Q.Dist = min(Room, Shape);
    
    if (Q.Dist == Room)
    {
        Q.MatId = 0;
    }
    else if (Q.Dist == Shape)
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
    
    if (MatId == 0) // room
    {
        Mat.Albedo = 0.8;
        Mat.Emission = 0;
        if (abs(P.y-5.0) < 0.3) Mat.Emission = 0;
    }
    else if (MatId == 1) // obj
    {
        Mat.Albedo = float3(0.8, 0.4, 0.3);
        Mat.Emission = 0;
        
        if (abs(P.y-1.0) < 0.1) Mat.Emission = 0;
    }
    else // invalid id (default)
    {
        Mat.Albedo = 0.8;
        Mat.Emission = 0.0;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 10.00*float3(0.3, 0.4, 0.5);
}
