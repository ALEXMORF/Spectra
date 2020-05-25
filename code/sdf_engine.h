#pragma once

#include "common.h"
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

internal void Win32Panic(char *Fmt, ...);

#include "d3d12_utils.h"
#include "ch_math.h"

#define WIDTH 1280
#define HEIGHT 720
#define BACKBUFFER_COUNT 2

struct input
{
    int Keys[256];
    b32 MouseDown;
    v2i MouseP;
    v2i MousedP;
    
    b32 MouseDataIsInitialized;
};

struct camera
{
    v3 P;
    quaternion Orientation;
};

struct engine
{
    ID3D12Device *Device;
    gpu_context Context;
    IDXGISwapChain3 *SwapChain;
    
    descriptor_arena UAVArena;
    
    pso PathTracePSO;
    pso TemporalFilterPSO;
    pso CalcVariancePSO;
    pso PackGBufferPSO;
    pso CorrelateHistoryPSO;
    pso SpatialFilterPSO;
    pso ApplyPrimaryShadingPSO;
    pso TaaPSO;
    pso ToneMapPSO;
    
    // pathtracer output
    texture LightTex;
    texture PositionTex;
    texture NormalTex;
    texture AlbedoTex;
    texture EmissionTex;
    
    // denoiser data
    texture PositionHistTex;
    texture NormalHistTex;
    texture LightHistTex;
    texture LumMomentTex;
    texture LumMomentHistTex;
    texture VarianceTex;
    texture NextVarianceTex;
    texture GBufferTex;
    texture PrevPixelIdTex;
    texture IntegratedLightTex;
    texture TempTex;
    
    // TAA data
    texture TaaHistTex;
    texture TaaOutputTex;
    
    texture OutputTex;
    texture BackBufferTexs[BACKBUFFER_COUNT];
    
    camera Camera;
    camera PrevCamera;
    int SwitchViewThreshold;
    
    float Time;
    int FrameIndex;
    b32 IsInitialized;
    
    void UpdateAndRender(HWND Window, input *Input);
};
