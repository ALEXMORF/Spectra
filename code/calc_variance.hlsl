#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1))"

RWTexture2D<float2> LumMomentTex: register(u0);
RWTexture2D<float> VarianceTex: register(u1);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float2 Moments = LumMomentTex[ThreadId];
    float Mean = Moments.x;
    float Mean2 = Moments.y;
    float Variance = abs(Mean2 - Mean*Mean);
    VarianceTex[ThreadId] = Variance;
}