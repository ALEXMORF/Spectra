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

float2x2 Rotate2(float Theta)
{
    float C = cos(Theta);
    float S = sin(Theta);
    return float2x2(C, -S,
                    S, C);
}

float sdBox(float3 P, float3 Radius)
{
    float3 Q = abs(P) - Radius;
    return length(max(Q, 0)) + min(max(max(Q.x, Q.y), Q.z), 0.0);
}

//point_query Map(float3 P, float Time);
//material MapMaterial(int MatId, float3 P, float Time);
//float3 Env(float3 Rd, float Time);
//float Fog(float Depth);

//#include "demo.hlsl"
#include "tunnel.hlsl"
//#include "sphere2.hlsl"
//#include "cornellbox.hlsl"
//#include "tron.hlsl"
//#include "coolbox.hlsl"
//#include "interior.hlsl"
