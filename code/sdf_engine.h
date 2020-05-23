#pragma once

#include "common.h"
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "d3d12_utils.h"

#define BACKBUFFER_COUNT 2

struct engine
{
    ID3D12Device *Device;
    ID3D12CommandAllocator *CmdAllocator;
    ID3D12GraphicsCommandList *CmdList;
    ID3D12CommandQueue *CmdQueue;
    IDXGISwapChain3 *SwapChain;
    
    descriptor_arena UAVArena;
    
    b32 IsInitialized;
    
    void UpdateAndRender(HWND Window);
};