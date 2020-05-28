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

//point_query Map(float3 P, float Time);
//material MapMaterial(int MatId, float3 P, float Time);
//float3 Env(float3 Rd, float Time);

#include "cornellbox.hlsl"
//#include "coolbox.hlsl"
