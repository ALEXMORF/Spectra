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
    
    bool IsRelaxed = true;
    float PrevDist = 0;
    float PrevStep = 0;
    float T = 0.0;
    int Iter = 0;
    for (Iter = 0; Iter < MAX_RAYMARCH_ITER && T < T_MAX; ++Iter)
    {
        point_query Q = Map(Ro + T*Rd, Time);
        
        if (IsRelaxed && abs(Q.Dist) + abs(PrevDist) <= PrevStep) // doesn't intersect
        {
            IsRelaxed = false;
            T -= PrevStep;
            Q = Map(Ro + T*Rd, Time);
        }
        
        if (Q.Dist < T_EPSILON)
        {
            MatId = Q.MatId;
            break;
        }
        
        float Scale = IsRelaxed? RAYMARCH_RELAXATION: 1.0;
        float Step = Scale * Q.Dist;
        T += Step;
        
        PrevDist = Q.Dist;
        PrevStep = Step;
    }
    
    hit Hit;
    Hit.T = T;
    Hit.MatId = MatId;
    return Hit;
}
