#pragma once
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DXOP(Value) ASSERT(SUCCEEDED(Value))

struct descriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    
    b32 IsValid();
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

struct pso
{
    ID3D12PipelineState *Handle;
    ID3D12RootSignature *RootSignature;
    
    void Release();
};

struct texture
{
    ID3D12Resource *Handle;
    D3D12_RESOURCE_STATES ResourceState;
    
    descriptor UAV;
    descriptor SRV;
};

struct frame_context
{
    ID3D12CommandAllocator *CmdAllocator;
    UINT64 FenceValue;
    ID3D12Fence *Fence;
    HANDLE FenceEvent;
};

#define FRAMES_IN_FLIGHT 2

struct gpu_context
{
    ID3D12Device *Device;
    ID3D12GraphicsCommandList *CmdList;
    ID3D12CommandQueue *CmdQueue;
    frame_context Frames[FRAMES_IN_FLIGHT];
    
    UINT BI;
    
    D3D12_RESOURCE_BARRIER CachedBarriers[16];
    int CachedBarrierCount;
    
    void Reset(UINT BackbufferIndex);
    void WaitForGpu(UINT NextBackbufferIndex);
    void FlushFramesInFlight();
    
    void UAVBarrier(texture *Tex);
    void TransitionBarrier(texture *Tex, D3D12_RESOURCE_STATES NewState);
    void FlushBarriers();
    void CopyResourceBarriered(texture *Dest, texture *Source);
    
    texture LoadTexture2D(char *Filename, DXGI_FORMAT Format, 
                          D3D12_RESOURCE_FLAGS Flags, 
                          D3D12_RESOURCE_STATES ResourceStates);
    
    void _PushToBarrierCache(D3D12_RESOURCE_BARRIER Barrier);
};

//
//
// GPU context

internal frame_context
InitFrameContext(ID3D12Device *D)
{
    frame_context Context = {};
    
    DXOP(D->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                   IID_PPV_ARGS(&Context.CmdAllocator)));
    
    DXOP(D->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Context.Fence)));
    Context.FenceEvent = CreateEventA(0, 0, FALSE, 0);
    
    return Context;
}

internal gpu_context
InitGPUContext(ID3D12Device *D)
{
    gpu_context Context = {};
    Context.Device = D;
    
    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
    CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    DXOP(D->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&Context.CmdQueue)));
    
    for (int FrameI = 0; FrameI < FRAMES_IN_FLIGHT; ++FrameI)
    {
        Context.Frames[FrameI] = InitFrameContext(D);
    }
    
    DXOP(D->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                              Context.Frames[0].CmdAllocator, 0,
                              IID_PPV_ARGS(&Context.CmdList)));
    DXOP(Context.CmdList->Close());
    
    return Context;
}

void
gpu_context::Reset(UINT CurrentBackbufferIndex)
{
    this->BI = CurrentBackbufferIndex;
    
    DXOP(Frames[BI].CmdAllocator->Reset());
    DXOP(CmdList->Reset(Frames[BI].CmdAllocator, 0));
}

void 
gpu_context::WaitForGpu(UINT NextBI)
{
    Frames[BI].FenceValue += 1;
    DXOP(CmdQueue->Signal(Frames[BI].Fence, Frames[BI].FenceValue));
    
    frame_context *NextFrame = Frames + NextBI;
    
    if (NextFrame->Fence->GetCompletedValue() < NextFrame->FenceValue)
    {
        DXOP(NextFrame->Fence->SetEventOnCompletion(NextFrame->FenceValue, NextFrame->FenceEvent));
        WaitForSingleObject(NextFrame->FenceEvent, INFINITE);
    }
}

void 
gpu_context::FlushFramesInFlight()
{
    for (int BufferI = 0; BufferI < FRAMES_IN_FLIGHT; ++BufferI)
    {
        frame_context *Frame = Frames + BufferI;
        
        if (Frame->Fence->GetCompletedValue() < Frame->FenceValue)
        {
            DXOP(Frame->Fence->SetEventOnCompletion(Frame->FenceValue, Frame->FenceEvent));
            WaitForSingleObject(Frame->FenceEvent, INFINITE);
        }
    }
}

void 
gpu_context::UAVBarrier(texture *Tex)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.UAV.pResource = Tex->Handle;
    
    _PushToBarrierCache(Barrier);
}

void 
gpu_context::TransitionBarrier(texture *Tex, D3D12_RESOURCE_STATES NewState)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = Tex->Handle;
    Barrier.Transition.StateBefore = Tex->ResourceState;
    Barrier.Transition.StateAfter = NewState;
    
    _PushToBarrierCache(Barrier);
    
    Tex->ResourceState = NewState;
}

void 
gpu_context::_PushToBarrierCache(D3D12_RESOURCE_BARRIER Barrier)
{
    CachedBarriers[CachedBarrierCount++] = Barrier;
    
    if (CachedBarrierCount == ARRAY_COUNT(CachedBarriers))
    {
        FlushBarriers();
    }
}

void
gpu_context::FlushBarriers()
{
    CmdList->ResourceBarrier(CachedBarrierCount, CachedBarriers);
    CachedBarrierCount = 0;
}

void
gpu_context::CopyResourceBarriered(texture *Dest, texture *Source)
{
    D3D12_RESOURCE_STATES SourceState = Source->ResourceState;
    D3D12_RESOURCE_STATES DestState = Dest->ResourceState;
    TransitionBarrier(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushBarriers();
    
    CmdList->CopyResource(Dest->Handle, Source->Handle);
    
    TransitionBarrier(Source, SourceState);
    TransitionBarrier(Dest, DestState);
    FlushBarriers();
}

//
//
// swapchain

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
    
    //NOTE(chen): tells windows to get the fuck out of my business
    //            I will handle resize/fullscreen myself goddamn it
    DXOP(Factory2->MakeWindowAssociation(Window, DXGI_MWA_NO_WINDOW_CHANGES|DXGI_MWA_NO_ALT_ENTER|DXGI_MWA_NO_PRINT_SCREEN));
    
    return SwapChain;
}

//
//
// descriptors

b32 
descriptor::IsValid()
{
    return GPUHandle.ptr != 0;
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

//
//
// pipeline states

void pso::Release()
{
    Handle->Release();
    RootSignature->Release();
    Handle = 0;
    RootSignature = 0;
}

internal b32
VerifyComputeShader(char *Filename, char *EntryPoint)
{
    FILE *File = fopen(Filename, "rb");
    if (!File)
    {
        Win32MessageBox("Shader Load Failure", MB_OK|MB_ICONERROR, 
                        "Failed to load shader %s", Filename);
        return false;
    }
    
    fseek(File, 0, SEEK_END);
    int FileSize = ftell(File);
    rewind(File);
    void *FileData = calloc(FileSize+1, 1);
    fread(FileData, 1, FileSize, File);
    fclose(File);
    
    //NOTE(chen): for whatever reason, d3d compiler's preprocessor is really
    //            bad at fetching include files. Numerously times it fails to
    //            open include files when the file are already there ...
    //
    //            this especially poses a problem with shader hotloading
    int MaxAttempt = 10;
    int Attempts = 0;
    ID3DBlob *CodeBlob; 
    ID3DBlob *ErrorBlob;
    while (FAILED(D3DCompile(FileData, FileSize, Filename,
                             0, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             EntryPoint, "cs_5_1", 0, 0,
                             &CodeBlob, &ErrorBlob)))
    {
        if (Attempts == MaxAttempt)
        {
            char *ErrorMsg = (char *)ErrorBlob->GetBufferPointer();
            Win32MessageBox("Shader Compile Error", MB_OK|MB_ICONERROR, "%s", ErrorMsg);
            ErrorBlob->Release();
            return false;
        }
        
        ErrorBlob->Release();
        Sleep(1); // try wait for file to come back online ....
        Attempts += 1;
    }
    
    CodeBlob->Release();
    free(FileData);
    
    return true;
}

internal pso
InitComputePSO(ID3D12Device *D, char *Filename, char *EntryPoint)
{
    pso PSO = {};
    
    FILE *File = fopen(Filename, "rb");
    if (!File)
    {
        Win32Panic("Failed to load shader file: %s", Filename);
        return PSO;
    }
    
    fseek(File, 0, SEEK_END);
    int FileSize = ftell(File);
    rewind(File);
    void *FileData = calloc(FileSize+1, 1);
    fread(FileData, 1, FileSize, File);
    fclose(File);
    
    //NOTE(chen): for whatever reason, d3d compiler's preprocessor is really
    //            bad at fetching include files. Numerously times it fails to
    //            open include files when the file are already there ...
    //
    //            this especially poses a problem with shader hotloading
    int MaxAttempt = 10;
    int Attempts = 0;
    ID3DBlob *CodeBlob; 
    ID3DBlob *ErrorBlob;
    while (FAILED(D3DCompile(FileData, FileSize, Filename,
                             0, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             EntryPoint, "cs_5_1", 0, 0,
                             &CodeBlob, &ErrorBlob)))
    {
        if (Attempts == MaxAttempt)
        {
            char *ErrorMsg = (char *)ErrorBlob->GetBufferPointer();
            Win32Panic("Shader compile error: %s", ErrorMsg);
            break;
        }
        
        ErrorBlob->Release();
        Sleep(1); // try wait for file to come back online ....
        Attempts += 1;
    }
    
    free(FileData);
    
    ID3D12RootSignatureDeserializer *RSExtractor = 0;
    DXOP(D3D12CreateRootSignatureDeserializer(CodeBlob->GetBufferPointer(),
                                              CodeBlob->GetBufferSize(), IID_PPV_ARGS(&RSExtractor)));
    const D3D12_ROOT_SIGNATURE_DESC *RSDesc = RSExtractor->GetRootSignatureDesc();
    
    ID3DBlob *RSBlob = 0;
    ID3DBlob *RSErrorBlob = 0;
    if (FAILED(D3D12SerializeRootSignature(RSDesc, 
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           &RSBlob, &RSErrorBlob)))
    {
        char *ErrorMsg = (char *)RSErrorBlob->GetBufferPointer();
        Win32Panic("Root Signature serialization error: %s", ErrorMsg);
    }
    DXOP(D->CreateRootSignature(0, RSBlob->GetBufferPointer(),
                                RSBlob->GetBufferSize(),
                                IID_PPV_ARGS(&PSO.RootSignature)));
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.pRootSignature = PSO.RootSignature;
    PSODesc.CS = {CodeBlob->GetBufferPointer(), CodeBlob->GetBufferSize()};
    DXOP(D->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSO.Handle)));
    
    RSBlob->Release();
    CodeBlob->Release();
    
    return PSO;
}

//
//
// boilerplates

internal D3D12_HEAP_PROPERTIES
DefaultHeapProps(D3D12_HEAP_TYPE Type)
{
    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type = Type;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    return HeapProps;
}

//
// 
// buffers

internal ID3D12Resource *
InitBuffer(ID3D12Device *D, size_t ByteCount,
           D3D12_HEAP_TYPE HeapType,
           D3D12_RESOURCE_STATES InitialState,
           D3D12_RESOURCE_FLAGS Flags)
{
    ID3D12Resource *Buffer = 0;
    
    D3D12_HEAP_PROPERTIES HeapProps = DefaultHeapProps(HeapType);
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Width = ByteCount;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc = {1, 0};
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = Flags;
    
    DXOP(D->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                    &ResourceDesc, InitialState, 0,
                                    IID_PPV_ARGS(&Buffer)));
    
    return Buffer;
}

//
//
// textures

internal texture
WrapTexture(ID3D12Resource *Handle, D3D12_RESOURCE_STATES ResourceState)
{
    texture Tex = {};
    Tex.ResourceState = ResourceState;
    Tex.Handle = Handle;
    return Tex;
}

internal texture
InitTexture2D(ID3D12Device *D, int Width, int Height, DXGI_FORMAT Format,
              D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES ResourceState)
{
    texture Tex = {};
    Tex.ResourceState = ResourceState;
    
    D3D12_HEAP_PROPERTIES HeapProps = DefaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ResourceDesc.Width = Width;
    ResourceDesc.Height = Height;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = Format;
    ResourceDesc.SampleDesc = {1, 0};
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.Flags = Flags;
    DXOP(D->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                    &ResourceDesc, ResourceState, 0,
                                    IID_PPV_ARGS(&Tex.Handle)));
    
    return Tex;
}

texture gpu_context::LoadTexture2D(char *Filename, DXGI_FORMAT Format,
                                   D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES ResourceState)
{
    int Width, Height, ChannelCount;
    u8 *ImageData = stbi_load(Filename, &Width, &Height, &ChannelCount, 0);
    
    //TODO(chen): this routine doesn't handle arb texel formats,
    //            assert this fact here
    ASSERT(Format == DXGI_FORMAT_R8G8B8A8_UNORM);
    ASSERT(ChannelCount == 4);
    
    texture Tex = InitTexture2D(Device, Width, Height, Format, Flags, D3D12_RESOURCE_STATE_COPY_DEST);
    
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};
    
    D3D12_RESOURCE_DESC ResourceDesc = Tex.Handle->GetDesc();
    Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &Footprint, 0, 0, 0);
    int RowPitch = Footprint.Footprint.RowPitch;
    
    size_t PaddedTexDataSize = Height * RowPitch;
    u32 *PaddedTexData = (u32 *)calloc(PaddedTexDataSize, 1);
    for (int Y = 0; Y < Height; ++Y)
    {
        for (int X = 0; X < Width; ++X)
        {
            PaddedTexData[X + Y*(RowPitch/sizeof(u32))] = ((u32 *)ImageData)[X + Y*Height];
        }
    }
    
    ID3D12Resource *StagingBuffer = InitBuffer(Device, 
                                               PaddedTexDataSize,
                                               D3D12_HEAP_TYPE_UPLOAD,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               D3D12_RESOURCE_FLAG_NONE);
    D3D12_RANGE ReadRange = {};
    void *MappedAddr = 0;
    DXOP(StagingBuffer->Map(0, &ReadRange, &MappedAddr));
    
    memcpy(MappedAddr, PaddedTexData, PaddedTexDataSize);
    
    D3D12_RANGE WriteRange = {0, PaddedTexDataSize};
    StagingBuffer->Unmap(0, &WriteRange);
    
    Reset(0);
    
    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestLocation.pResource = Tex.Handle;
    DestLocation.SubresourceIndex = 0;
    
    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SourceLocation.pResource = StagingBuffer;
    SourceLocation.PlacedFootprint = Footprint;
    
    CmdList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, 0);
    
    TransitionBarrier(&Tex, ResourceState);
    FlushBarriers();
    
    DXOP(CmdList->Close());
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    WaitForGpu(0);
    
    StagingBuffer->Release();
    stbi_image_free(ImageData);
    free(PaddedTexData);
    
    return Tex;
}

internal void
AssignUAV(ID3D12Device *D, texture *Tex, descriptor_arena *Arena)
{
    Tex->UAV = Arena->PushDescriptor();
    D->CreateUnorderedAccessView(Tex->Handle, 0, 0, 
                                 Tex->UAV.CPUHandle);
}

internal void
AssignSRV(ID3D12Device *D, texture *Tex, descriptor_arena *Arena)
{
    Tex->SRV = Arena->PushDescriptor();
    D->CreateShaderResourceView(Tex->Handle, 0, Tex->SRV.CPUHandle);
}
