#define RS "DescriptorTable(UAV(u0))"

RWTexture2D<float4> OutputTex: register(u0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadID: SV_DispatchThreadID)
{
    OutputTex[ThreadID] = 0.0;
}