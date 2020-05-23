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
        
        D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
        CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        DXOP(D->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&CmdQueue)));
        
        SwapChain = CreateSwapChain(CmdQueue, Window,
                                    BACKBUFFER_COUNT);
        
        DXOP(D->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&CmdAllocator)));
        
        DXOP(D->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  CmdAllocator, 0,
                                  IID_PPV_ARGS(&CmdList)));
        DXOP(CmdList->Close());
        
        UAVArena = InitDescriptorArena(D, 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        ClearPSO = InitComputePSO(D, L"../code/clear.hlsl", "main");
        
        IsInitialized = true;
    }
    
    DXOP(CmdAllocator->Reset());
    DXOP(CmdList->Reset(CmdAllocator, 0));
    
    ASSERT(!"TODO(chen): Impl frame");
    ASSERT(!"TODO(chen): Submit frame & wait");
    ASSERT(!"TODO(chen): Present");
}