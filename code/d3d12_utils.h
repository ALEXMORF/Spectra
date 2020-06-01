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
    descriptor RTV;
};

struct frame_context
{
    ID3D12CommandAllocator *CmdAllocator;
    UINT64 FenceValue;
    ID3D12Fence *Fence;
    HANDLE FenceEvent;
};

struct gpu_context
{
    ID3D12Device *Device;
    ID3D12GraphicsCommandList *CmdList;
    ID3D12CommandQueue *CmdQueue;
    
    frame_context *Frames;
    int FramesInFlight;
    
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
    
    void Upload(texture *Tex, void *Data);
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
InitGPUContext(ID3D12Device *D, int FramesInFlight)
{
    gpu_context Context = {};
    Context.Device = D;
    Context.FramesInFlight = FramesInFlight;
    
    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
    CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    DXOP(D->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&Context.CmdQueue)));
    
    Context.Frames = (frame_context *)calloc(FramesInFlight, sizeof(frame_context));
    for (int FrameI = 0; FrameI < Context.FramesInFlight; ++FrameI)
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
    for (int BufferI = 0; BufferI < FramesInFlight; ++BufferI)
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
    if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    else
    {
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    }
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

struct file
{
    void *Data;
    size_t Size;
};

file ReadEntireFile(char *Path, int Padding)
{
    file File = {};
    
    FILE *F = fopen(Path, "rb");
    if (F)
    {
        fseek(F, 0, SEEK_END);
        File.Size = ftell(F);
        rewind(F);
        File.Data = calloc(File.Size+Padding, 1);
        fread(File.Data, 1, File.Size, F);
        fclose(F);
    }
    
    return File;
}

file ReadTextFile(char *Path)
{
    return ReadEntireFile(Path, 1);
}

file ReadBinaryFile(char *Path)
{
    return ReadEntireFile(Path, 0);
}

internal ID3DBlob *
CompileShader(file File, char *Filename, char *EntryPoint, char *Target,
              b32 *HasError_Out)
{
    bool Failed = false;
    
    //NOTE(chen): for whatever reason, d3d compiler's preprocessor is really
    //            bad at fetching include files. Numerously times it fails to
    //            open include files when the file are already there ...
    //
    //            this especially poses a problem with shader hotloading
    int MaxAttempt = 10;
    int Attempts = 0;
    ID3DBlob *CodeBlob; 
    ID3DBlob *ErrorBlob;
    while (FAILED(D3DCompile(File.Data, File.Size, Filename,
                             0, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             EntryPoint, Target, 0, 0,
                             &CodeBlob, &ErrorBlob)))
    {
        if (Attempts == MaxAttempt)
        {
            Failed = true;
            break;
        }
        
        ErrorBlob->Release();
        Sleep(1); // try wait for file to come back online ....
        Attempts += 1;
    }
    
    *HasError_Out = Failed;
    return Failed? ErrorBlob: CodeBlob;
}

internal b32
VerifyComputeShader(char *Filename, char *EntryPoint)
{
    file File = ReadTextFile(Filename);
    if (!File.Data)
    {
        Win32MessageBox("Shader Load Failure", MB_OK|MB_ICONERROR, 
                        "Failed to load shader %s", Filename);
        return false;
    }
    
    b32 HasError = 0;
    ID3DBlob *Blob = CompileShader(File, Filename, EntryPoint, "cs_5_1", &HasError);
    free(File.Data);
    
    if (HasError)
    {
        char *ErrorMsg = (char *)Blob->GetBufferPointer();
        Win32MessageBox("Shader Compile Error", MB_OK|MB_ICONERROR, "%s", ErrorMsg);
        Blob->Release();
        return false;
    }
    
    Blob->Release();
    return true;
}

internal ID3D12RootSignature *
ReflectRootSignature(ID3D12Device *D, ID3DBlob *CodeBlob, 
                     ID3DBlob **RSErrorBlob_Out)
{
    ID3D12RootSignature *Result = 0;
    
    ID3D12RootSignatureDeserializer *RSExtractor = 0;
    DXOP(D3D12CreateRootSignatureDeserializer(CodeBlob->GetBufferPointer(),
                                              CodeBlob->GetBufferSize(), IID_PPV_ARGS(&RSExtractor)));
    const D3D12_ROOT_SIGNATURE_DESC *RSDesc = RSExtractor->GetRootSignatureDesc();
    
    ID3DBlob *RSBlob = 0;
    ID3DBlob *RSErrorBlob = 0;
    if (SUCCEEDED(D3D12SerializeRootSignature(RSDesc, 
                                              D3D_ROOT_SIGNATURE_VERSION_1,
                                              &RSBlob, &RSErrorBlob)))
    {
        DXOP(D->CreateRootSignature(0, RSBlob->GetBufferPointer(),
                                    RSBlob->GetBufferSize(),
                                    IID_PPV_ARGS(&Result)));
        RSBlob->Release();
    }
    else
    {
        *RSErrorBlob_Out = RSErrorBlob;
    }
    
    return Result;
}

internal pso
InitComputePSO(ID3D12Device *D, char *Filename, char *EntryPoint)
{
    pso PSO = {};
    
    file File = ReadTextFile(Filename);
    if (!File.Data)
    {
        Win32Panic("Failed to load shader file: %s", Filename);
        return PSO;
    }
    
    b32 HasError = 0;
    ID3DBlob *Blob = CompileShader(File, Filename, EntryPoint, "cs_5_1", &HasError);
    free(File.Data);
    if (HasError)
    {
        Win32Panic("Shader compile error: %s", Blob->GetBufferPointer());
        Blob->Release();
        return PSO;
    }
    
    ID3DBlob *CodeBlob = Blob;
    
    ID3DBlob *RSErrorBlob = 0;
    PSO.RootSignature = ReflectRootSignature(D, CodeBlob, &RSErrorBlob);
    if (!PSO.RootSignature)
    {
        char *ErrorMsg = (char *)RSErrorBlob->GetBufferPointer();
        Win32Panic("Root Signature serialization error: %s", ErrorMsg);
        RSErrorBlob->Release();
    }
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.pRootSignature = PSO.RootSignature;
    PSODesc.CS = {CodeBlob->GetBufferPointer(), CodeBlob->GetBufferSize()};
    DXOP(D->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSO.Handle)));
    
    CodeBlob->Release();
    
    return PSO;
}

typedef void edit_graphics_pso(D3D12_GRAPHICS_PIPELINE_STATE_DESC *PSODesc);

internal pso
InitGraphicsPSO(ID3D12Device *D, char *Filename, 
                D3D12_INPUT_ELEMENT_DESC *InputElements, int InputElementCount,
                edit_graphics_pso *EditGraphicsPSO = 0)
{
    pso PSO = {};
    
    file File = ReadTextFile(Filename);
    if (!File.Data)
    {
        Win32Panic("Failed to load shader file: %s", Filename);
        return PSO;
    }
    
    b32 HasError = 0;
    ID3DBlob *VSBlob = CompileShader(File, Filename, "VS", "vs_5_1", &HasError);
    if (HasError)
    {
        Win32Panic("VS Shader compile error: %s", VSBlob->GetBufferPointer());
        VSBlob->Release();
        return PSO;
    }
    
    ID3DBlob *PSBlob = CompileShader(File, Filename, "PS", "ps_5_1", &HasError);
    if (HasError)
    {
        Win32Panic("PS Shader compile error: %s", PSBlob->GetBufferPointer());
        PSBlob->Release();
        return PSO;
    }
    
    free(File.Data);
    
    ID3DBlob *RSErrorBlob = 0;
    PSO.RootSignature = ReflectRootSignature(D, VSBlob, &RSErrorBlob);
    if (!PSO.RootSignature)
    {
        char *ErrorMsg = (char *)RSErrorBlob->GetBufferPointer();
        Win32Panic("Root Signature serialization error: %s", ErrorMsg);
        RSErrorBlob->Release();
    }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.pRootSignature = PSO.RootSignature;
    PSODesc.VS = {VSBlob->GetBufferPointer(), VSBlob->GetBufferSize()};
    PSODesc.PS = {PSBlob->GetBufferPointer(), PSBlob->GetBufferSize()};
    PSODesc.BlendState.AlphaToCoverageEnable = FALSE;
    PSODesc.BlendState.IndependentBlendEnable = FALSE;
    PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    PSODesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    PSODesc.SampleMask = UINT_MAX;
    PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    PSODesc.RasterizerState.FrontCounterClockwise = TRUE;
    PSODesc.DepthStencilState.DepthEnable = FALSE;
    PSODesc.DepthStencilState.StencilEnable = FALSE;
    PSODesc.InputLayout.pInputElementDescs = InputElements;
    PSODesc.InputLayout.NumElements = InputElementCount;
    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.NumRenderTargets = 1;
    PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    PSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    PSODesc.SampleDesc.Count = 1; // one sample per pixel
    PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    if (EditGraphicsPSO)
    {
        EditGraphicsPSO(&PSODesc);
    }
    
    DXOP(D->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO.Handle)));
    
    VSBlob->Release();
    PSBlob->Release();
    
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

internal void
Upload(ID3D12Resource *Buffer, void *Data, size_t ByteCount)
{
    D3D12_RANGE ReadRange = {};
    void *MappedAddr = 0;
    DXOP(Buffer->Map(0, &ReadRange, &MappedAddr));
    memcpy(MappedAddr, Data, ByteCount);
    D3D12_RANGE WriteRange = {0, ByteCount};
    Buffer->Unmap(0, &WriteRange);
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

int GetChannelCount(DXGI_FORMAT Format)
{
    int ChannelCount = 0;
    
    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        {
            ChannelCount = 4;
        } break;
        
        case DXGI_FORMAT_R8_UNORM:
        {
            ChannelCount = 1;
        } break;
        
        default:
        {
            ASSERT(!"unhandled DXGI format");
        } break;
    }
    
    return ChannelCount;
}

int GetBytesPerTexel(DXGI_FORMAT Format)
{
    int Count = 0;
    
    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        {
            Count = 4;
        } break;
        
        case DXGI_FORMAT_R8_UNORM:
        {
            Count = 1;
        } break;
        
        default:
        {
            ASSERT(!"unhandled DXGI format");
        } break;
    }
    
    return Count;
}

void gpu_context::Upload(texture *Tex, void *TexData)
{
    D3D12_RESOURCE_DESC Desc = Tex->Handle->GetDesc();
    int Width = int(Desc.Width);
    int Height = int(Desc.Height);
    DXGI_FORMAT Format = Desc.Format;
    
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};
    
    Device->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, 0, 0, 0);
    int RowPitch = Footprint.Footprint.RowPitch;
    int BytesPerPixel = GetBytesPerTexel(Format);
    int BytesPerRow = BytesPerPixel * Width;
    
    size_t PaddedTexDataSize = Height * RowPitch;
    u8 *PaddedTexData = (u8 *)calloc(PaddedTexDataSize, 1);
    
    u8 *Reader = (u8 *)TexData;
    u8 *Writer = PaddedTexData;
    for (int Y = 0; Y < Height; ++Y)
    {
        for (int X = 0; X < BytesPerRow; ++X)
        {
            Writer[X] = Reader[X];
        }
        Writer += RowPitch;
        Reader += BytesPerRow;
    }
    
    ID3D12Resource *StagingBuffer = InitBuffer(Device, 
                                               PaddedTexDataSize,
                                               D3D12_HEAP_TYPE_UPLOAD,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               D3D12_RESOURCE_FLAG_NONE);
    ::Upload(StagingBuffer, PaddedTexData, PaddedTexDataSize);
    
    Reset(0);
    
    D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
    DestLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestLocation.pResource = Tex->Handle;
    DestLocation.SubresourceIndex = 0;
    
    D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
    SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SourceLocation.pResource = StagingBuffer;
    SourceLocation.PlacedFootprint = Footprint;
    
    D3D12_RESOURCE_STATES TexState = Tex->ResourceState;
    
    TransitionBarrier(Tex, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushBarriers();
    
    CmdList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, 0);
    
    TransitionBarrier(Tex, TexState);
    FlushBarriers();
    
    DXOP(CmdList->Close());
    ID3D12CommandList *CmdLists[] = {CmdList};
    CmdQueue->ExecuteCommandLists(1, CmdLists);
    WaitForGpu(0);
    
    StagingBuffer->Release();
    free(PaddedTexData);
}

texture gpu_context::LoadTexture2D(char *Filename, DXGI_FORMAT Format,
                                   D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES ResourceState)
{
    int Width, Height, ChannelCount;
    u8 *ImageData = stbi_load(Filename, &Width, &Height, &ChannelCount, 0);
    ASSERT(ChannelCount == GetChannelCount(Format));
    
    texture Tex = InitTexture2D(Device, Width, Height, Format, Flags, ResourceState);
    Upload(&Tex, ImageData);
    
    stbi_image_free(ImageData);
    
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

internal void
AssignRTV(ID3D12Device *D, texture *Tex, descriptor_arena *Arena)
{
    Tex->RTV = Arena->PushDescriptor();
    D->CreateRenderTargetView(Tex->Handle, 0, Tex->RTV.CPUHandle);
}
