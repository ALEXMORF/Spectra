static float2 gSeed;

void SeedRand(float InitialSeed)
{
    gSeed = InitialSeed;
}

float2 Rand2() {
    gSeed += float2(-1,1);
    return float2(frac(sin(dot(gSeed.xy ,float2(12.9898,78.233))) * 43758.5453),
                  frac(cos(dot(gSeed.xy ,float2(4.898,7.23))) * 23421.631));
};
