#pragma once

#include "common.h"
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

internal void Win32Panic(char *Fmt, ...);
internal void Win32MessageBox(char *Title, UINT Type, char *Fmt, ...);

#include "d3d12_utils.h"
#include "ch_math.h"

#include "ui.h"

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
    gpu_context Context;
    IDXGISwapChain3 *SwapChain;
    int Width;
    int Height;
    
    ui_system UISystem;
    
    descriptor_arena DescriptorArena;
    
    pso PrimaryPSO;
    pso PathTracePSO;
    pso ComputeDisocclusionPSO;
    pso TemporalFilterPSO;
    pso CalcVariancePSO;
    pso PackGBufferPSO;
    pso CorrelateHistoryPSO;
    pso SpatialFilterPSO;
    pso ApplyPrimaryShadingPSO;
    pso TaaPSO;
    pso ToneMapPSO;
    
    // pathtracer output
    texture NoisyLightTex;
    texture PositionTex;
    texture NormalTex;
    texture AlbedoTex;
    texture EmissionTex;
    texture RayDirTex;
    texture BlueNoiseTexs[64];
    texture DisocclusionTex;
    
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
    v2 HaltonSequence[8];
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
    
    void UpdateAndRender(HWND Window, int ClientWidth, int ClientHeight,
                         input *Input, b32 NeedsReload, f32 dT);
    void InitOrResizeWindowDependentResources();
};
