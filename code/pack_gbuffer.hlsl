#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), RootConstants(num32BitConstants=3, b0)"

RWTexture2D<float4> PositionTex: register(u0);
RWTexture2D<float4> NormalTex: register(u1);
RWTexture2D<float4> GBufferTex: register(u2);

struct context
{
    float3 CamP;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float Depth = length(Context.CamP - PositionTex[ThreadId].xyz);
    float3 Normal = NormalTex[ThreadId].xyz;
    GBufferTex[ThreadId] = float4(Normal, Depth);
}