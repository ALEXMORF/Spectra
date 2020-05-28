struct hit
{
    float T;
    int MatId;
};

float3 CalcGradient(float3 P, float Time)
{
    float2 E = float2(0, 0.0001);
    
    float3 Gradient;
    Gradient.x = Map(P + E.yxx, Time).Dist - Map(P - E.yxx, Time).Dist;
    Gradient.y = Map(P + E.xyx, Time).Dist - Map(P - E.xyx, Time).Dist;
    Gradient.z = Map(P + E.xxy, Time).Dist - Map(P - E.xxy, Time).Dist;
    
    return normalize(Gradient);
}

hit RayMarch(float3 Ro, float3 Rd, float Time)
{
    int MatId = -1;
    float T = 0.0;
    int Iter = 0;
    for (Iter = 0; Iter < 512 && T < T_MAX; ++Iter)
    {
        point_query Q = Map(Ro + T*Rd, Time);
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
