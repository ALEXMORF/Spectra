#include "sdf_engine.h"

void
engine::UpdateAndRender(HWND Window)
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
        
        OutputTex = InitTexture2D(D, WIDTH, HEIGHT, 
                                  DXGI_FORMAT_R8G8B8A8_UNORM,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        OutputTex.UAV = UAVArena.PushDescriptor();
        D->CreateUnorderedAccessView(OutputTex.Handle, 0, 0, 
                                     OutputTex.UAV.CPUHandle);
        
        IsInitialized = true;
    }
    
    Context.Reset();
    ID3D12GraphicsCommandList *CmdList = Context.CmdList;
    ID3D12CommandQueue *CmdQueue = Context.CmdQueue;
    
    int ThreadGroupCountX = (WIDTH-1)/32 + 1;
    int ThreadGroupCountY = (HEIGHT-1)/32 + 1;
    int Width = WIDTH;
    int Height = HEIGHT;
    
    CmdList->SetPipelineState(PathTracePSO.Handle);
    CmdList->SetComputeRootSignature(PathTracePSO.RootSignature);
    CmdList->SetDescriptorHeaps(1, &UAVArena.Heap);
    CmdList->SetComputeRootDescriptorTable(0, OutputTex.UAV.GPUHandle);
    CmdList->SetComputeRoot32BitConstants(1, 1, &Width, 0);
    CmdList->SetComputeRoot32BitConstants(1, 1, &Height, 1);
    CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
    
    Context.UAVBarrier(&OutputTex);
    Context.FlushBarriers();
    
    UINT BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    texture *BackBufferTex = BackBufferTexs + BackBufferIndex;
    
    Context.TransitionBarrier(&OutputTex, 
                              D3D12_RESOURCE_STATE_COPY_SOURCE);
    Context.TransitionBarrier(BackBufferTex, 
                              D3D12_RESOURCE_STATE_COPY_DEST);
    Context.FlushBarriers();
    
    CmdList->CopyResource(BackBufferTex->Handle, OutputTex.Handle);
    
    Context.TransitionBarrier(&OutputTex, 
                              D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.TransitionBarrier(BackBufferTex, 
                              D3D12_RESOURCE_STATE_PRESENT);
    Context.FlushBarriers();
    
    DXOP(CmdList->Close());
    
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    
    SwapChain->Present(1, 0);
    
    Context.WaitForGpu();
}