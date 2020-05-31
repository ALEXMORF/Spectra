#define BIND(D, Id) Q.Dist = min(Q.Dist, D); if (Q.Dist == D) Q.MatId = Id;

float hash(in float x)
{
    return frac(sin(13.15*x)*932.3);
}

float hash2(in float2 p)
{
    return hash(dot(p, float2(91.3, 151.16)));
}

float hash3(in float3 p)
{
    return hash(dot(p, float3(91.3, 151.16, 72.15)));
}

float noise3(in float3 p)
{
    float3 ipos = floor(p);
    float3 fpos = frac(p);
    
    float a = hash3(ipos + float3(0, 0, 0));
    float b = hash3(ipos + float3(1, 0, 0));
    float c = hash3(ipos + float3(0, 1, 0));
    float d = hash3(ipos + float3(1, 1, 0));
    float e = hash3(ipos + float3(0, 0, 1));
    float f = hash3(ipos + float3(1, 0, 1));
    float g = hash3(ipos + float3(0, 1, 1));
    float h = hash3(ipos + float3(1, 1, 1));
    
    float3 t = smoothstep(0, 1, fpos);
    
    return lerp(lerp(lerp(a, b, t.x), lerp(c, d, t.x), t.y),
                lerp(lerp(e, f, t.x), lerp(g, h, t.x), t.y),
                t.z);
}

float fbm3(in float3 p)
{
    float res = 0.0;
    float amp = 0.5;
    float freq = 2.0;
    for (int i = 0; i < 6; ++i)
    {
        res += amp*noise3(freq*p);
        amp *= 0.5;
        freq *= 2.0;
    }
    return res;
}

float noise2(in float2 p)
{
    float2 ipos = floor(p);
    float2 fpos = frac(p);
    
    float a = hash2(ipos + float2(0, 0));
    float b = hash2(ipos + float2(1, 0));
    float c = hash2(ipos + float2(0, 1));
    float d = hash2(ipos + float2(1, 1));
    
    float2 t = smoothstep(0, 1, fpos);
    
    return lerp(lerp(a, b, t.x), lerp(c, d, t.x), t.y);
}

float fbm2(in float2 p)
{
    float res = 0.0;
    float amp = 0.5;
    float freq = 2.0;
    for (int i = 0; i < 6; ++i)
    {
        res += amp*noise2(freq*p);
        amp *= 0.5;
        freq *= 2.0;
    }
    return res;
}

point_query Map(float3 P, float Time)
{
    point_query Q;
    Q.Dist = 10e31;
    Q.MatId = -1;
    
    float Floor = P.y + 1.0;
    float Sphere = length(P) - 1.0;
    float Tunnel = -(length(P.xy) - 5.0);
    
    BIND(Floor, 0);
    BIND(Sphere, 1);
    BIND(Tunnel, 2);
    
    return Q;
}

material MapMaterial(int MatId, float3 P, float Time)
{
    material Mat;
    Mat.Albedo = 0.9;
    Mat.Emission = 0;
    
    if (MatId == 0) // floor
    {
        Mat.Albedo = 0.1;
        
        float2 Deterioration = 0.1*fbm2(5.0*P.xz);
        if (abs(P.x) < 0.3+Deterioration.x && 
            frac(0.1*P.z) < 0.8+Deterioration.y) 
        {
            Mat.Albedo = float3(0.8, 0.8, 0.1);
        }
    }
    else if (MatId == 1) // sphere
    {
        Mat.Emission = 0;
    }
    else if (MatId == 2) // tunnel
    {
        Mat.Albedo = float3(0.7, 0.4, 0.3);
        if (abs(P.y) < 0.2) Mat.Emission = 5;
    }
    
    return Mat;
}

float3 Env(float3 Rd, float Time)
{
    return 0.0 * float3(0.3, 0.4, 0.5);
}

float Fog(float Depth)
{
    return clamp(1.0 - Depth / 100.0, 0, 1);
}