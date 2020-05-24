#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), RootConstants(num32BitConstants=1, b0)"

RWTexture2D<float4> InputTex: register(u0);
RWTexture2D<float4> LightHistTex: register(u1);
RWTexture2D<float4> FilteredTex: register(u2);

struct context
{
    int FrameIndex;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float4 Input = InputTex[ThreadId];
    
    //TODO(chen): proper reprojection
    //TODO(chen): proper geometry consistency test
    float4 Hist = LightHistTex[ThreadId];
    
    float Alpha = 0.1;
    if (Context.FrameIndex == 0)
    {
        Alpha = 1.0;
    }
    
    float4 Filtered = lerp(Hist, Input, Alpha);
    FilteredTex[ThreadId] = Filtered;
}