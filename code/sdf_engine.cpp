#include "sdf_engine.h"

void
engine::UpdateAndRender(HWND Window, input *Input)
{
    if (!IsInitialized)
    {
        ID3D12Debug *Debug = 0;
        DXOP(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug)));
        Debug->EnableDebugLayer();
        
        DXOP(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0, 
                               IID_PPV_ARGS(&Device)));
        ID3D12Device *D = Device;
        
        Context = InitGPUContext(D);
        
        SwapChain = CreateSwapChain(Context.CmdQueue, Window, BACKBUFFER_COUNT);
        
        for (int I = 0; I < BACKBUFFER_COUNT; ++I)
        {
            ID3D12Resource *BackBuffer = 0;
            SwapChain->GetBuffer(I, IID_PPV_ARGS(&BackBuffer));
            BackBufferTexs[I] = WrapTexture(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
        }
        
        UAVArena = InitDescriptorArena(D, 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        PathTracePSO = InitComputePSO(D, L"../code/pathtrace.hlsl", "main");
        TemporalFilterPSO = InitComputePSO(D, L"../code/temporal_filter.hlsl", "main");
        CalcVariancePSO = InitComputePSO(D, L"../code/calc_variance.hlsl", "main");
        SpatialFilterPSO = InitComputePSO(D, L"../code/spatial_filter.hlsl", "main");
        ToneMapPSO = InitComputePSO(D, L"../code/tonemap.hlsl", "main");
        
        LightTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                 DXGI_FORMAT_R16G16B16A16_FLOAT,
                                 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        LightTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(LightTex.Handle, 0, 0, 
                                     LightTex.UAV.CPUHandle);
        
        PositionTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        PositionTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(PositionTex.Handle, 0, 0, 
                                     PositionTex.UAV.CPUHandle);
        
        NormalTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R16G16B16A16_FLOAT,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        NormalTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(NormalTex.Handle, 0, 0, 
                                     NormalTex.UAV.CPUHandle);
        
        PositionHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                        DXGI_FORMAT_R32G32B32A32_FLOAT,
                                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        PositionHistTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(PositionHistTex.Handle, 0, 0, 
                                     PositionHistTex.UAV.CPUHandle);
        
        NormalHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                      DXGI_FORMAT_R16G16B16A16_FLOAT,
                                      D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        NormalHistTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(NormalHistTex.Handle, 0, 0, 
                                     NormalHistTex.UAV.CPUHandle);
        
        LightHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                     DXGI_FORMAT_R16G16B16A16_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        LightHistTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(LightHistTex.Handle, 0, 0, 
                                     LightHistTex.UAV.CPUHandle);
        
        LumMomentTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                     DXGI_FORMAT_R16G16_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        LumMomentTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(LumMomentTex.Handle, 0, 0, 
                                     LumMomentTex.UAV.CPUHandle);
        
        LumMomentHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                         DXGI_FORMAT_R16G16_FLOAT,
                                         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        LumMomentHistTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(LumMomentHistTex.Handle, 0, 0, 
                                     LumMomentHistTex.UAV.CPUHandle);
        
        VarianceTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                    DXGI_FORMAT_R32_FLOAT,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        VarianceTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(VarianceTex.Handle, 0, 0, 
                                     VarianceTex.UAV.CPUHandle);
        
        NextVarianceTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                        DXGI_FORMAT_R32_FLOAT,
                                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        NextVarianceTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(NextVarianceTex.Handle, 0, 0, 
                                     NextVarianceTex.UAV.CPUHandle);
        
        IntegratedLightTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                           DXGI_FORMAT_R16G16B16A16_FLOAT,
                                           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        IntegratedLightTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(IntegratedLightTex.Handle, 0, 0, 
                                     IntegratedLightTex.UAV.CPUHandle);
        
        TempTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                DXGI_FORMAT_R16G16B16A16_FLOAT,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TempTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(TempTex.Handle, 0, 0, 
                                     TempTex.UAV.CPUHandle);
        
        OutputTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R8G8B8A8_UNORM,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        OutputTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(OutputTex.Handle, 0, 0, 
                                     OutputTex.UAV.CPUHandle);
        
        Camera.P = {0.0f, 1.2f, -4.0f};
        Camera.Orientation = Quaternion();
        
        IsInitialized = true;
    }
    
    // camera orientation
    if (Input->MouseDown)
    {
        v2 MousedP = {f32(Input->MousedP.X) / f32(WIDTH), f32(Input->MousedP.Y) / f32(HEIGHT)};
        quaternion XRot = Quaternion(YAxis(), MousedP.X * Pi32);
        Camera.Orientation = XRot * Camera.Orientation;
        
        v3 LocalXAxis = Rotate(XAxis(), Camera.Orientation);
        quaternion YRot = Quaternion(LocalXAxis, MousedP.Y * Pi32);
        Camera.Orientation = YRot * Camera.Orientation;
    }
    
    // camera translation
    v3 dP = {};
    if (Input->Keys['W']) dP.Z += 1.0f;
    if (Input->Keys['S']) dP.Z -= 1.0f;
    if (Input->Keys['A']) dP.X -= 1.0f;
    if (Input->Keys['D']) dP.X += 1.0f;
    dP = Normalize(dP);
    dP = Rotate(dP, Camera.Orientation);
    f32 Speed = 0.1f;
    Camera.P += Speed * dP;
    
    Context.Reset();
    ID3D12GraphicsCommandList *CmdList = Context.CmdList;
    ID3D12CommandQueue *CmdQueue = Context.CmdQueue;
    
    int ThreadGroupCountX = (WIDTH-1)/32 + 1;
    int ThreadGroupCountY = (HEIGHT-1)/32 + 1;
    int Width = WIDTH;
    int Height = HEIGHT;
    
    v3 CamOffset = Rotate(ZAxis(), Camera.Orientation);
    v3 CamAt = Camera.P + CamOffset;
    
    // path tracing
    {
        CmdList->SetPipelineState(PathTracePSO.Handle);
        CmdList->SetComputeRootSignature(PathTracePSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &UAVArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(3, 1, &Width, 0);
        CmdList->SetComputeRoot32BitConstants(3, 1, &Height, 1);
        CmdList->SetComputeRoot32BitConstants(3, 1, &FrameIndex, 2);
        CmdList->SetComputeRoot32BitConstants(3, 3, &Camera.P, 4);
        CmdList->SetComputeRoot32BitConstants(3, 3, &CamAt, 8);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&LightTex);
        Context.UAVBarrier(&PositionTex);
        Context.UAVBarrier(&NormalTex);
        Context.FlushBarriers();
    }
    
    // temporal filter
    {
        quaternion PrevCameraInvOrientation = Conjugate(PrevCamera.Orientation);
        
        CmdList->SetPipelineState(TemporalFilterPSO.Handle);
        CmdList->SetComputeRootSignature(TemporalFilterPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &UAVArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, LightHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, PositionHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(6, NormalHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(7, LumMomentHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(8, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(9, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(9, 3, &PrevCamera.P, 1);
        CmdList->SetComputeRoot32BitConstants(9, 4, &PrevCameraInvOrientation, 4);
        CmdList->SetComputeRoot32BitConstants(9, 3, &Camera.P, 8);
        CmdList->SetComputeRoot32BitConstants(9, 1, &Width, 12);
        CmdList->SetComputeRoot32BitConstants(9, 1, &Height, 13);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&IntegratedLightTex);
        Context.UAVBarrier(&LumMomentTex);
        Context.FlushBarriers();
        
        Context.CopyResourceBarriered(&LightHistTex, &IntegratedLightTex);
        Context.CopyResourceBarriered(&LumMomentHistTex, &LumMomentTex);
        Context.CopyResourceBarriered(&PositionHistTex, &PositionTex);
        Context.CopyResourceBarriered(&NormalHistTex, &NormalTex);
    }
    
    // calc variance
    {
        CmdList->SetPipelineState(CalcVariancePSO.Handle);
        CmdList->SetComputeRootSignature(CalcVariancePSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &UAVArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, VarianceTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(5, 3, &Camera.P, 0);
        
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&VarianceTex);
        Context.FlushBarriers();
    }
    
    // spatial filter
    {
        int Iterations = 6;
        // this gaurantees the end result ends up in IntegratedLightTex
        ASSERT(Iterations % 2 == 0); 
        
        texture *PingPongs[2] = {&IntegratedLightTex, &TempTex};
        texture *Variances[2] = {&VarianceTex, &NextVarianceTex};
        
        for (int Depth = 0; Depth < Iterations; ++Depth)
        {
            int FilterStride = 1 << Depth;
            
            CmdList->SetPipelineState(SpatialFilterPSO.Handle);
            CmdList->SetComputeRootSignature(SpatialFilterPSO.RootSignature);
            CmdList->SetDescriptorHeaps(1, &UAVArena.Heap);
            CmdList->SetComputeRootDescriptorTable(0, PingPongs[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(1, PingPongs[1]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(2, PositionTex.UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(3, NormalTex.UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(4, Variances[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(5, Variances[1]->UAV.GPUHandle);
            CmdList->SetComputeRoot32BitConstants(6, 3, &Camera.P, 0);
            CmdList->SetComputeRoot32BitConstants(6, 1, &Depth, 3);
            CmdList->SetComputeRoot32BitConstants(6, 1, &FilterStride, 4);
            CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
            
            Context.UAVBarrier(PingPongs[1]);
            Context.UAVBarrier(Variances[1]);
            Context.FlushBarriers();
            
            // swap
            texture *Temp = PingPongs[1];
            PingPongs[1] = PingPongs[0];
            PingPongs[0] = Temp;
            
            // swap
            texture *VarTemp = Variances[1];
            Variances[1] = Variances[0];
            Variances[0] = VarTemp;
        }
    }
    
    CmdList->SetPipelineState(ToneMapPSO.Handle);
    CmdList->SetComputeRootSignature(ToneMapPSO.RootSignature);
    CmdList->SetComputeRootDescriptorTable(0, IntegratedLightTex.UAV.GPUHandle);
    CmdList->SetComputeRootDescriptorTable(1, OutputTex.UAV.GPUHandle);
    CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
    
    Context.UAVBarrier(&OutputTex);
    Context.FlushBarriers();
    
    UINT BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    texture *BackBufferTex = BackBufferTexs + BackBufferIndex;
    Context.CopyResourceBarriered(BackBufferTex, &OutputTex);
    
    DXOP(CmdList->Close());
    
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    
    SwapChain->Present(1, 0);
    
    Context.WaitForGpu();
    
    FrameIndex += 1;
    PrevCamera = Camera;
}