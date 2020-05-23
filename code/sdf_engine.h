#pragma once

#include "common.h"
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

internal void Win32Panic(char *Fmt, ...);

#include "d3d12_utils.h"

#define WIDTH 1280
#define HEIGHT 720
#define BACKBUFFER_COUNT 2

struct engine
{
    ID3D12Device *Device;
    gpu_context Context;
    IDXGISwapChain3 *SwapChain;
    
    descriptor_arena UAVArena;
    
    pso PathTracePSO;
    
    texture OutputTex;
    texture BackBufferTexs[BACKBUFFER_COUNT];
    
    b32 IsInitialized;
    
    void UpdateAndRender(HWND Window);
};
