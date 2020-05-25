#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), RootConstants(num32BitConstants=3, b0)"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> HistTex: register(u1);
RWTexture2D<float4> OutputTex: register(u2);
RWTexture2D<float2> PrevPixelIdTex: register(u3);

struct context
{
    int FrameIndex;
    int Width;
    int Height;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float2 PrevPixelId = PrevPixelIdTex[ThreadId];
    
    if (Context.FrameIndex != 0 &&
        all(PrevPixelId >= 0.0 && PrevPixelId <= float2(Context.Width-1, Context.Height-1)))
    {
        float2 Interp = frac(PrevPixelId);
        int2 PrevPixelCoord = floor(PrevPixelId);
        
        float4 FilteredHist = 0;
        for (int dY = 0; dY <= 1; ++dY)
        {
            for (int dX = 0; dX <= 1; ++dX)
            {
                int2 TapCoord = PrevPixelCoord + int2(dX, dY);
                float2 Bilinear = lerp(1.0-Interp, Interp, float2(dX, dY));
                float W = Bilinear.x * Bilinear.y;
                FilteredHist += W * HistTex[TapCoord];
            }
        }
        
        float4 NeighborMin = 10e31;
        float4 NeighborMax = -10e31;
        for (int dY = -1; dY <= 1; ++dY)
        {
            for (int dX = -1; dX <= 1; ++dX)
            {
                int2 TapCoord = ThreadId + int2(dX, dY);
                float4 Tap = InputTex[TapCoord];
                NeighborMin = min(NeighborMin, Tap);
                NeighborMax = max(NeighborMax, Tap);
            }
        }
        FilteredHist = clamp(FilteredHist, NeighborMin, NeighborMax);
        
        float Alpha = 0.1;
        OutputTex[ThreadId] = lerp(FilteredHist, InputTex[ThreadId], Alpha);
    }
    else
    {
        OutputTex[ThreadId] = InputTex[ThreadId];
    }
}