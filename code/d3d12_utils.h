#pragma once

#define DXOP(Value) ASSERT(SUCCEEDED(Value))

struct descriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

struct descriptor_arena
{
    ID3D12Device *D;
    
    ID3D12DescriptorHeap *Heap;
    D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
    int Count;
    int Capacity;
    
    descriptor PushDescriptor();
};

internal IDXGISwapChain3 *
CreateSwapChain(ID3D12CommandQueue *CmdQueue,
                HWND Window,
                int BackbufferCount)
{
    IDXGISwapChain3 *SwapChain = 0;
    
    IDXGIFactory2 *Factory2 = 0;
    DXOP(CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory2)));
    
    IDXGISwapChain1 *SwapChain1 = 0;
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width = 0; // uses window width
    SwapChainDesc.Height = 0; // uses window height
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.Stereo = FALSE;
    SwapChainDesc.SampleDesc.Count = 1; // Single sample per pixel
    SwapChainDesc.SampleDesc.Quality = 0; 
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = BackbufferCount;
    SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags = 0;
    DXOP(Factory2->CreateSwapChainForHwnd(CmdQueue, // for d3d12, must be command queue instead of device
                                          Window,
                                          &SwapChainDesc,
                                          0, // doesn't support fullscreen
                                          0,
                                          &SwapChain1));
    DXOP(SwapChain1->QueryInterface(IID_PPV_ARGS(&SwapChain)));
    
    return SwapChain;
}

internal descriptor_arena
InitDescriptorArena(ID3D12Device *D, int Count, 
                    D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
{
    descriptor_arena Arena = {};
    Arena.D = D;
    Arena.Capacity = Count;
    Arena.HeapType = HeapType;
    
    D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
    HeapDesc.Type = HeapType;
    HeapDesc.NumDescriptors = Count;
    HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DXOP(D->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Arena.Heap)));
    
    return Arena;
}

descriptor 
descriptor_arena::PushDescriptor()
{
    ASSERT(Count < Capacity);
    
    descriptor Descriptor = {};
    
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHeapStart = Heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHeapStart = Heap->GetGPUDescriptorHandleForHeapStart();
    UINT DescriptorSize = D->GetDescriptorHandleIncrementSize(HeapType);
    
    int CurrIndex = Count;
    Descriptor.GPUHandle = GPUHeapStart;
    Descriptor.GPUHandle.ptr += CurrIndex * DescriptorSize;
    Descriptor.CPUHandle = CPUHeapStart;
    Descriptor.CPUHandle.ptr += CurrIndex * DescriptorSize;
    
    Count += 1;
    
    return Descriptor;
}