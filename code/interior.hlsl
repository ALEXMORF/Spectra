float hash(in float x)
{
    return frac(sin(13.15*x)*932.3);
}

float box(float3 p, float3 b)
{
    float3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

#define GROUND_ID 0
#define LIGHT_BULB_ID 1
#define TABLE_ID 2
#define ROOM_ID 3

point_query Map(float3 p, float Time)
{  
    float light_bulb = length(p - float3(4.25, 4.2, 0)) - 2.0;
    float ground = p.y;
    float room = max(-box(p - float3(1.8, 1, 3), float3(0.5, 0.9, 1)), max(box(p - float3(0, 1, 0), float3(3, 2, 2.7)), -box(p - float3(0, 1, 0), 0.8*float3(3, 2, 3))));
    float table = box(p - float3(1.9, 0.6, 0.8), float3(0.5, 0.05, 0.6));
    float leg1 = box(p - float3(1.45, 0.3, 1.35), float3(0.05, 0.3, 0.05));
    float leg2 = box(p - float3(2.35, 0.3, 1.35), float3(0.05, 0.3, 0.05));
    float leg3 = box(p - float3(1.45, 0.3, 0.25), float3(0.05, 0.3, 0.05));
    float leg4 = box(p - float3(2.35, 0.3, 0.25), float3(0.05, 0.3, 0.05));
    table = min(min(min(min(table, leg1), leg2), leg3), leg4);
    float d = min(table, min(room, min(light_bulb, ground)));
    
    int id = -1;
    if (d == light_bulb)
    {
        id = LIGHT_BULB_ID;
    }
    else if (d == ground)
    {
        id = GROUND_ID;
    }
    else if (d == table)
    {
        id = TABLE_ID;
    }
    else if (d == room)
    {
		id = ROOM_ID;
    }
    
    point_query Q;
    Q.Dist = d;
    Q.MatId = id;
    return Q;
}

float hash3(in float3 p)
{
    return hash(dot(p, float3(91.3, 151.16, 72.15)));
}

float noise(in float3 p)
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

float fbm(in float3 p)
{
    float res = 0.0;
    float amp = 0.5;
    float freq = 2.0;
    for (int i = 0; i < 6; ++i)
    {
        res += amp*noise(freq*p);
        amp *= 0.5;
        freq *= 2.0;
    }
    return res;
}

material MapMaterial(int id, float3 p, float Time)
{
    material mat;
    
    if (id == GROUND_ID)
    {
        if (dot(p,p) > 3.5*3.5)
        {
            mat.Albedo = 0.5;
            mat.Emission = 0;
        }
        else
        {
            float3 col1 = float3(0.33, 0.18, 0.09);
            float3 col2 = 0.3*float3(0.33, 0.18, 0.09);
            
            mat.Albedo = lerp(col1, col2, fbm(float3(p.x, p.y, 10.0*p.z)));
            mat.Emission = 0;
        }
    }
    else if (id == LIGHT_BULB_ID)
    {
        mat.Albedo = 0.5;
        mat.Emission = 0.000001;
    }
    else if (id == ROOM_ID)
    {
        float3 col1 = float3(0.8, 0.4, 0.2);
        float3 col2 = 0.7*col1;
        
        mat.Albedo = lerp(col1, col2, fbm(p));
        mat.Emission = 0;
    }
    else if (id == TABLE_ID)
    {
        mat.Albedo = 0.9;
        mat.Emission = 0;
    }
    else
    {
        mat.Albedo = float3(1, 0, 1);
        mat.Emission = 0;
    }
    
    return mat;
}

float3 Env(float3 Rd, float Time)
{
    return 10.0;
}

float Fog(float Depth)
{
    return 1.0;
}