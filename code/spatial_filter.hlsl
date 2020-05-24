#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), RootConstants(num32BitConstants=5, b0)"

RWTexture2D<float4> InputBuffer: register(u0);
RWTexture2D<float4> OutputBuffer: register(u1);

RWTexture2D<float4> PositionBuffer: register(u2);
RWTexture2D<float4> NormalBuffer: register(u3);

struct context
{
    float3 CamP;
    int Depth;
    int Stride;
};

ConstantBuffer<context> Context: register(b0);

float DepthWeight(float CenterDepth, float SampleDepth)
{
    return abs(1.0 - CenterDepth/SampleDepth) < 0.01? 1.0: 0.0;
}

float NormalWeight(float3 CenterNormal, float3 SampleNormal)
{
    return pow(max(0.0, dot(CenterNormal, SampleNormal)), 32.0);
}

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float CenterDepth = length(Context.CamP - PositionBuffer[ThreadId].xyz);
    float3 CenterNormal = NormalBuffer[ThreadId].xyz;
    
    const float Kernel[3] = {3.0/8.0, 1.0/4.0, 1/16.0};
    
    float4 Filtered = 0.0;
    float TotalContrib = 0.0;
    for (int dY = -1; dY <= 1; ++dY)
    {
        for (int dX = -1; dX <= 1; ++dX)
        {
            int2 Coord = int2(ThreadId) + Context.Stride * int2(dX, dY);
            float4 Tap = InputBuffer[Coord];
            
            float W = Kernel[abs(dX)]*Kernel[abs(dY)];
            float TapDepth = length(Context.CamP - PositionBuffer[Coord].xyz);
            float3 TapNormal = NormalBuffer[Coord].xyz;
            W *= DepthWeight(CenterDepth, TapDepth);
            W *= NormalWeight(CenterNormal, TapNormal);
            
            Filtered += W * Tap;
            TotalContrib += W;
        }
    }
    
    if (TotalContrib > 0.0)
    {
        Filtered /= TotalContrib;
    }
    
    OutputBuffer[ThreadId] = Filtered;
}