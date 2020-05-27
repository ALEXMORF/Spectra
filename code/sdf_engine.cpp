#include "sdf_engine.h"

void
engine::UpdateAndRender(HWND Window, input *Input, b32 NeedsReload)
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
        
        DescriptorArena = InitDescriptorArena(D, 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        PathTracePSO = InitComputePSO(D, "../code/pathtrace.hlsl", "main");
        TemporalFilterPSO = InitComputePSO(D, "../code/temporal_filter.hlsl", "main");
        CalcVariancePSO = InitComputePSO(D, "../code/calc_variance.hlsl", "main");
        PackGBufferPSO = InitComputePSO(D, "../code/pack_gbuffer.hlsl", "main");
        CorrelateHistoryPSO = InitComputePSO(D, "../code/correlate_history.hlsl", "main");
        SpatialFilterPSO = InitComputePSO(D, "../code/spatial_filter.hlsl", "main");
        ApplyPrimaryShadingPSO = InitComputePSO(D, "../code/apply_primary_shading.hlsl", "main");
        TaaPSO = InitComputePSO(D, "../code/taa.hlsl", "main");
        ToneMapPSO = InitComputePSO(D, "../code/tonemap.hlsl", "main");
        
        for (int I = 0; I < 64; ++I)
        {
            char BlueNoiseFilename[256];
            snprintf(BlueNoiseFilename, sizeof(BlueNoiseFilename),
                     "../data/textures/HDR_RGBA_%d.png", I);
            BlueNoiseTexs[I] = Context.LoadTexture2D(BlueNoiseFilename, 
                                                     DXGI_FORMAT_R8G8B8A8_UNORM,
                                                     D3D12_RESOURCE_FLAG_NONE,
                                                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            AssignSRV(D, BlueNoiseTexs+I, &DescriptorArena);
        }
        
        LightTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                 DXGI_FORMAT_R16G16B16A16_FLOAT,
                                 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &LightTex, &DescriptorArena);
        
        PositionTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &PositionTex, &DescriptorArena);
        
        NormalTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R16G16B16A16_FLOAT,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &NormalTex, &DescriptorArena);
        
        AlbedoTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R16G16B16A16_FLOAT,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &AlbedoTex, &DescriptorArena);
        
        EmissionTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                    DXGI_FORMAT_R16G16B16A16_FLOAT,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &EmissionTex, &DescriptorArena);
        
        RayDirTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R32G32B32A32_FLOAT,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &RayDirTex, &DescriptorArena);
        
        PositionHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                        DXGI_FORMAT_R32G32B32A32_FLOAT,
                                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &PositionHistTex, &DescriptorArena);
        
        NormalHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                      DXGI_FORMAT_R16G16B16A16_FLOAT,
                                      D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &NormalHistTex, &DescriptorArena);
        
        LightHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                     DXGI_FORMAT_R16G16B16A16_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &LightHistTex, &DescriptorArena);
        
        LumMomentTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                     DXGI_FORMAT_R16G16_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &LumMomentTex, &DescriptorArena);
        
        LumMomentHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                         DXGI_FORMAT_R16G16_FLOAT,
                                         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &LumMomentHistTex, &DescriptorArena);
        
        VarianceTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                    DXGI_FORMAT_R32_FLOAT,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &VarianceTex, &DescriptorArena);
        
        NextVarianceTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                        DXGI_FORMAT_R32_FLOAT,
                                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &NextVarianceTex, &DescriptorArena);
        
        GBufferTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                   DXGI_FORMAT_R32G32B32A32_FLOAT,
                                   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &GBufferTex, &DescriptorArena);
        
        PrevPixelIdTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                       DXGI_FORMAT_R32G32_FLOAT,
                                       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &PrevPixelIdTex, &DescriptorArena);
        
        IntegratedLightTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                           DXGI_FORMAT_R16G16B16A16_FLOAT,
                                           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &IntegratedLightTex, &DescriptorArena);
        
        TempTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                DXGI_FORMAT_R16G16B16A16_FLOAT,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &TempTex, &DescriptorArena);
        
        TaaOutputTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                     DXGI_FORMAT_R16G16B16A16_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &TaaOutputTex, &DescriptorArena);
        
        TaaHistTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                   DXGI_FORMAT_R16G16B16A16_FLOAT,
                                   D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &TaaHistTex, &DescriptorArena);
        
        OutputTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R8G8B8A8_UNORM,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        AssignUAV(D, &OutputTex, &DescriptorArena);
        
        Camera.P = {0.0f, 1.2f, -4.0f};
        Camera.Orientation = Quaternion();
        
        IsInitialized = true;
    }
    
    if (NeedsReload)
    {
        ID3D12Device *D = Context.Device;
        
        b32 ShadersAreValid = true;
        if (ShadersAreValid) ShadersAreValid = VerifyComputeShader("../code/pathtrace.hlsl", "main");
        if (ShadersAreValid) ShadersAreValid = VerifyComputeShader("../code/apply_primary_shading.hlsl", "main");
        
        if (ShadersAreValid)
        {
            PathTracePSO = InitComputePSO(D, "../code/pathtrace.hlsl", "main");
            ApplyPrimaryShadingPSO = InitComputePSO(D, "../code/apply_primary_shading.hlsl", "main");
        }
        
        FrameIndex = 0;
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
    
    if (Input->Keys['N']) SwitchViewThreshold -= 10;
    if (Input->Keys['M']) SwitchViewThreshold += 10;
    if (SwitchViewThreshold < 0) SwitchViewThreshold = 0;
    if (SwitchViewThreshold >= Width) SwitchViewThreshold = Width-1;
    
    // path tracing
    {
        CmdList->SetPipelineState(PathTracePSO.Handle);
        CmdList->SetComputeRootSignature(PathTracePSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(6, BlueNoiseTexs[0].SRV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(7, 1, &Width, 0);
        CmdList->SetComputeRoot32BitConstants(7, 1, &Height, 1);
        CmdList->SetComputeRoot32BitConstants(7, 1, &FrameIndex, 2);
        CmdList->SetComputeRoot32BitConstants(7, 3, &Camera.P, 4);
        CmdList->SetComputeRoot32BitConstants(7, 3, &CamAt, 8);
        CmdList->SetComputeRoot32BitConstants(7, 1, &Time, 11);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&LightTex);
        Context.UAVBarrier(&PositionTex);
        Context.UAVBarrier(&NormalTex);
        Context.UAVBarrier(&AlbedoTex);
        Context.UAVBarrier(&EmissionTex);
        Context.UAVBarrier(&RayDirTex);
        Context.FlushBarriers();
    }
    
    // pack gbuffer
    {
        quaternion CameraInvOrientation = Conjugate(Camera.Orientation);
        
        CmdList->SetPipelineState(PackGBufferPSO.Handle);
        CmdList->SetComputeRootSignature(PackGBufferPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, GBufferTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(3, 3, &Camera.P, 0);
        CmdList->SetComputeRoot32BitConstants(3, 4, &CameraInvOrientation, 4);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&GBufferTex);
        Context.FlushBarriers();
    }
    
    // correlate history pixels
    {
        quaternion PrevCameraInvOrientation = Conjugate(PrevCamera.Orientation);
        
        CmdList->SetPipelineState(CorrelateHistoryPSO.Handle);
        CmdList->SetComputeRootSignature(CorrelateHistoryPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(2, 3, &PrevCamera.P, 0);
        CmdList->SetComputeRoot32BitConstants(2, 4, &PrevCameraInvOrientation, 4);
        CmdList->SetComputeRoot32BitConstants(2, 1, &Width, 8);
        CmdList->SetComputeRoot32BitConstants(2, 1, &Height, 9);
        CmdList->SetComputeRoot32BitConstants(2, 1, &FrameIndex, 10);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&PrevPixelIdTex);
        Context.FlushBarriers();
    }
    
    // temporal filter
    {
        quaternion CurrCameraInvOrientation = Conjugate(Camera.Orientation);
        
        CmdList->SetPipelineState(TemporalFilterPSO.Handle);
        CmdList->SetComputeRootSignature(TemporalFilterPSO.RootSignature);
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, LightHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, NormalTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(5, PositionHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(6, NormalHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(7, LumMomentHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(8, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(9, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(10, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(10, 3, &Camera.P, 1);
        CmdList->SetComputeRoot32BitConstants(10, 4, &CurrCameraInvOrientation, 4);
        CmdList->SetComputeRoot32BitConstants(10, 1, &Width, 8);
        CmdList->SetComputeRoot32BitConstants(10, 1, &Height, 9);
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
        CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
        CmdList->SetComputeRootDescriptorTable(0, LumMomentTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, VarianceTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, GBufferTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 3, &Camera.P, 0);
        
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
            CmdList->SetDescriptorHeaps(1, &DescriptorArena.Heap);
            CmdList->SetComputeRootDescriptorTable(0, PingPongs[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(1, PingPongs[1]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(2, GBufferTex.UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(3, Variances[0]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(4, Variances[1]->UAV.GPUHandle);
            CmdList->SetComputeRootDescriptorTable(5, LightHistTex.UAV.GPUHandle);
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
    
    // apply primary shading
    {
        CmdList->SetPipelineState(ApplyPrimaryShadingPSO.Handle);
        CmdList->SetComputeRootSignature(ApplyPrimaryShadingPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, PositionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, AlbedoTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, EmissionTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(4, RayDirTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(5, 1, &Time, 0);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&IntegratedLightTex);
        Context.FlushBarriers();
    }
    
    // TAA
    {
        CmdList->SetPipelineState(TaaPSO.Handle);
        CmdList->SetComputeRootSignature(TaaPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, IntegratedLightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, TaaHistTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, TaaOutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, PrevPixelIdTex.UAV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 1, &FrameIndex, 0);
        CmdList->SetComputeRoot32BitConstants(4, 1, &Width, 1);
        CmdList->SetComputeRoot32BitConstants(4, 1, &Height, 2);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&TaaOutputTex);
        Context.FlushBarriers();
        
        Context.CopyResourceBarriered(&TaaHistTex, &TaaOutputTex);
    }
    
    Context.UAVBarrier(&LightHistTex);
    Context.FlushBarriers();
    
    // tonemap
    {
        CmdList->SetPipelineState(ToneMapPSO.Handle);
        CmdList->SetComputeRootSignature(ToneMapPSO.RootSignature);
        CmdList->SetComputeRootDescriptorTable(0, TaaOutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(1, OutputTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(2, LightTex.UAV.GPUHandle);
        CmdList->SetComputeRootDescriptorTable(3, BlueNoiseTexs[0].SRV.GPUHandle);
        CmdList->SetComputeRoot32BitConstants(4, 1, &SwitchViewThreshold, 0);
        CmdList->SetComputeRoot32BitConstants(4, 1, &FrameIndex, 1);
        CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        
        Context.UAVBarrier(&OutputTex);
        Context.FlushBarriers();
    }
    
    UINT BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    texture *BackBufferTex = BackBufferTexs + BackBufferIndex;
    Context.CopyResourceBarriered(BackBufferTex, &OutputTex);
    
    DXOP(CmdList->Close());
    
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    
    SwapChain->Present(1, 0);
    
    Context.WaitForGpu();
    
    Time += 1.0f/60.0f;
    FrameIndex += 1;
    PrevCamera = Camera;
}