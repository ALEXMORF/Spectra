#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1))"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> OutputTex: register(u1);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float4 Col = InputTex[ThreadId];
    
    Col = Col/(1.0+Col);
    Col = sqrt(Col);
    
    OutputTex[ThreadId] = Col;
}