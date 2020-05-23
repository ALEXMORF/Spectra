#include "sdf_engine.h"

#define DXOP(Value) ASSERT(SUCCEEDED(Value))

internal void
EngineUpdateAndRender(engine *Engine, HWND Window)
{
    if (!Engine->IsInitialized)
    {
        ID3D12Debug *Debug = 0;
        DXOP(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug)));
        Debug->EnableDebugLayer();
        
        DXOP(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0, 
                               IID_PPV_ARGS(&Engine->Device)));
        ID3D12Device *D = Engine->Device;
        
        D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
        CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        DXOP(D->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&Engine->CmdQueue)));
        
        IDXGIFactory2 *Factory2 = 0;
        CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory2));
        
        IDXGISwapChain1 *SwapChain1 = 0;
        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
        SwapChainDesc.Width = 0; // uses window width
        SwapChainDesc.Height = 0; // uses window height
        SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SwapChainDesc.Stereo = FALSE;
        SwapChainDesc.SampleDesc.Count = 1; // Single sample per pixel
        SwapChainDesc.SampleDesc.Quality = 0; 
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.BufferCount = BACKBUFFER_COUNT;
        SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        SwapChainDesc.Flags = 0;
        Factory2->CreateSwapChainForHwnd(Engine->CmdQueue, // for d3d12, must be command queue instead of device
                                         Window,
                                         &SwapChainDesc,
                                         0, // doesn't support fullscreen
                                         0,
                                         &SwapChain1);
        SwapChain1->QueryInterface(IID_PPV_ARGS(&Engine->SwapChain));
        
        DXOP(D->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&Engine->CmdAllocator)));
        
        DXOP(D->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  Engine->CmdAllocator, 0,
                                  IID_PPV_ARGS(&Engine->CmdList)));
        DXOP(Engine->CmdList->Close());
        
        Engine->IsInitialized = true;
    }
}