internal ui_system
InitUISystem(gpu_context *Context, descriptor_arena *DescriptorArena)
{
    ui_system System = {};
    
    stbtt_fontinfo FontInfo = {};
    
    char *FontFilePath = "../data/liberation-mono.ttf";
    FILE *FontFile = fopen(FontFilePath, "rb");
    if (!FontFile)
    {
        Win32Panic("Failed to load font file located at %s", FontFilePath);
    }
    fseek(FontFile, 0, SEEK_END);
    int FileSize = ftell(FontFile);
    rewind(FontFile);
    u8 *FileData = (u8 *)calloc(FileSize, 1);
    fread(FileData, 1, FileSize, FontFile);
    fclose(FontFile);
    
    stbtt_InitFont(&FontInfo, FileData, 0);
    
    int BeginCharI = 32;
    int EndCharI = 126;
    int FontHeight = 50;
    
    f32 Scale = stbtt_ScaleForPixelHeight(&FontInfo, f32(FontHeight));
    int Ascent, Descent, LineGap;
    stbtt_GetFontVMetrics(&FontInfo, &Ascent, &Descent, &LineGap);
    int Baseline = int(f32(Ascent) * Scale);
    
    //NOTE(chen): ensure the font is monospace
    int AdvanceWidth, LeftBearing;
    stbtt_GetCodepointHMetrics(&FontInfo, BeginCharI, &AdvanceWidth, &LeftBearing);
    for (int CharI = BeginCharI; CharI <= EndCharI; ++CharI)
    {
        int CurrAdvanceWidth, CurrLeftBearing;
        stbtt_GetCodepointHMetrics(&FontInfo, CharI, &CurrAdvanceWidth, &CurrLeftBearing);
        ASSERT(AdvanceWidth == CurrAdvanceWidth);
    }
    int FontWidth = int(f32(AdvanceWidth) * Scale);
    
    int CharCount = EndCharI - BeginCharI + 1;
    int CharXCount = int(SquareRoot(f32(CharCount)));
    int CharYCount = Ceil(f32(CharCount) / f32(CharXCount));
    
    // allocate bitmap adaptively based on font metrics
    int AtlasWidth = CharXCount * FontWidth;
    int AtlasHeight = CharYCount * FontHeight;
    u8 *AtlasData = (u8 *)calloc(AtlasWidth*AtlasHeight, 1);
    
    // bake characters
    for (int CharI = BeginCharI; CharI <= EndCharI; ++CharI)
    {
        char Char = char(CharI);
        
        int X0, Y0, X1, Y1;
        stbtt_GetCodepointBitmapBox(&FontInfo, Char, Scale, Scale, &X0, &Y0, &X1, &Y1);
        
        int Index = CharI - BeginCharI;
        int X = (Index % CharXCount) * FontWidth + X0;
        int Y = (Index / CharXCount) * FontHeight + (Baseline + Y0);
        
        u8 *Out = AtlasData + X + Y*AtlasWidth;
        stbtt_MakeCodepointBitmap(&FontInfo, Out, 
                                  X1-X0, Y1-Y0, AtlasWidth, 
                                  Scale, Scale, CharI);
    }
    free(FileData);
    
    System.BeginCharI = BeginCharI;
    System.EndCharI = EndCharI;
    System.CharXCount = CharXCount;
    System.CharYCount = CharYCount;
    System.FontWidth = FontWidth;
    System.FontHeight = FontHeight;
    System.AtlasWidth = AtlasWidth;
    System.AtlasHeight = AtlasHeight;
    System.FontAtlas = InitTexture2D(Context->Device, 
                                     AtlasWidth, AtlasHeight,
                                     DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_NONE,
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context->Upload(&System.FontAtlas, AtlasData);
    AssignSRV(Context->Device, &System.FontAtlas, DescriptorArena);
    free(AtlasData);
    
    D3D12_INPUT_ELEMENT_DESC TextVertInputElements[] = {
        {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TYPE", 0, DXGI_FORMAT_R32G32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    
    auto Edit = [](D3D12_GRAPHICS_PIPELINE_STATE_DESC *PSODesc) {
        PSODesc->DepthStencilState.DepthEnable = false;
        PSODesc->RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        
        D3D12_RENDER_TARGET_BLEND_DESC *RTBlendDesc = PSODesc->BlendState.RenderTarget;
        RTBlendDesc->BlendEnable = TRUE;
        RTBlendDesc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
        RTBlendDesc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        RTBlendDesc->BlendOp = D3D12_BLEND_OP_ADD;
        RTBlendDesc->SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        RTBlendDesc->DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        RTBlendDesc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
    };
    System.RasterizeTextPSO = InitGraphicsPSO(Context->Device, 
                                              "../code/rasterize_text.hlsl", 
                                              TextVertInputElements,
                                              ARRAY_COUNT(TextVertInputElements),
                                              Edit);
    
    return System;
}

void 
ui_system::SetErrorMessage(gpu_context *Context, char *String)
{
    int StringLen = int(strlen(String));
    if (StringLen == 0)
    {
        UIVertCount = 0;
        if (UIVB)
        {
            Context->FlushFramesInFlight();
            UIVB->Release();
            UIVB = 0;
        }
        
        return;
    }
    
    int LineCount = 1;
    for (int I = 0; I < StringLen; ++I)
    {
        if (String[I] == '\n')  
        {
            LineCount += 1;
        }
    }
    
    v2 FontDim = V2(f32(FontWidth)/1280.0f, f32(FontHeight)/720.0f);
    v2 Offset = {-1.0f, -1.0f + f32(LineCount)*FontDim.Y};
    
    UIVertCount = 6 + 6 * StringLen;
    ui_vertex *TextVertices = (ui_vertex *)calloc(UIVertCount, sizeof(ui_vertex));
    
    // build verts
    int Cursor = 0;
    
    // build background verts
    {
        v2 TopLeftP = Offset + V2(0.0f, FontDim.Y);
        v2 BotRightP = Offset + V2(2.0f, (f32(LineCount)) * -FontDim.Y);
        
        TextVertices[Cursor++] = {TopLeftP, TopLeftP, 1};
        TextVertices[Cursor++] = {V2(BotRightP.X, TopLeftP.Y), V2(BotRightP.X, TopLeftP.Y), 1};
        TextVertices[Cursor++] = {V2(TopLeftP.X, BotRightP.Y), V2(TopLeftP.X, BotRightP.Y), 1};
        TextVertices[Cursor++] = {V2(TopLeftP.X, BotRightP.Y), V2(TopLeftP.X, BotRightP.Y), 1};
        TextVertices[Cursor++] = {BotRightP, BotRightP, 1};
        TextVertices[Cursor++] = {V2(BotRightP.X, TopLeftP.Y), V2(BotRightP.X, TopLeftP.Y), 1};
    }
    
    // build text verts
    for (int CharI = 0; CharI < StringLen; ++CharI)
    {
        int Code = String[CharI];
        
        if (Code >= BeginCharI && Code <= EndCharI)
        {
            int Index = Code - BeginCharI;
            
            int CodePointX = (Index % CharXCount) * FontWidth;
            int CodePointY = (Index / CharXCount) * FontHeight;
            
            f32 U = f32(CodePointX) / f32(AtlasWidth);
            f32 V = f32(CodePointY) / f32(AtlasHeight);
            f32 UVWidth = f32(FontWidth) / f32(AtlasWidth);
            f32 UVHeight = f32(FontHeight) / f32(AtlasHeight);
            
            v2 TopLeftP = Offset;
            v2 BotRightP = TopLeftP + V2(FontDim.X, -FontDim.Y);
            
            v2 TopLeftUV = V2(U, V);
            v2 BotRightUV = TopLeftUV + V2(UVWidth, UVHeight);
            
            TextVertices[Cursor++] = {TopLeftP, TopLeftUV, 0};
            TextVertices[Cursor++] = {V2(BotRightP.X, TopLeftP.Y), V2(BotRightUV.X, TopLeftUV.Y), 0};
            TextVertices[Cursor++] = {V2(TopLeftP.X, BotRightP.Y), V2(TopLeftUV.X, BotRightUV.Y), 0};
            TextVertices[Cursor++] = {V2(TopLeftP.X, BotRightP.Y), V2(TopLeftUV.X, BotRightUV.Y), 0};
            TextVertices[Cursor++] = {BotRightP, BotRightUV, 0};
            TextVertices[Cursor++] = {V2(BotRightP.X, TopLeftP.Y), V2(BotRightUV.X, TopLeftUV.Y), 0};
            
            Offset.X += FontDim.X;
        }
        else if (Code == '\n')
        {
            Offset.X = -1.0f;
            Offset.Y -= FontDim.Y;
        }
        else
        {
            ASSERT(!"Unhandled character");
        }
    }
    
    // upload verts
    if (UIVB)
    {
        Context->FlushFramesInFlight();
        UIVB->Release();
        UIVB = 0;
    }
    
    if (StringLen > 0)
    {
        size_t UIVertSize = UIVertCount * sizeof(ui_vertex);
        UIVB = InitBuffer(Context->Device, UIVertSize,
                          D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ,
                          D3D12_RESOURCE_FLAG_NONE);
        Upload(UIVB, TextVertices, UIVertSize);
    }
    
    free(TextVertices);
}

void
ui_system::DrawMessage(gpu_context *Context, texture RenderTarget)
{
    if (UIVertCount == 0) return;
    
    size_t UIVertSize = UIVertCount * sizeof(ui_vertex);
    
    int TargetWidth = int(RenderTarget.Handle->GetDesc().Width);
    int TargetHeight = RenderTarget.Handle->GetDesc().Height;
    
    D3D12_VIEWPORT Viewport = {};
    Viewport.Width = f32(TargetWidth);
    Viewport.Height = f32(TargetHeight);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    D3D12_RECT ScissorRect = {};
    ScissorRect.right = LONG(TargetWidth);
    ScissorRect.bottom = LONG(TargetHeight);
    
    ID3D12GraphicsCommandList *CmdList = Context->CmdList;
    CmdList->SetPipelineState(RasterizeTextPSO.Handle);
    CmdList->SetGraphicsRootSignature(RasterizeTextPSO.RootSignature);
    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    
    CmdList->SetGraphicsRootDescriptorTable(0, FontAtlas.SRV.GPUHandle);
    CmdList->OMSetRenderTargets(1, &RenderTarget.RTV.CPUHandle, 0, 0);
    D3D12_VERTEX_BUFFER_VIEW VBView = {};
    VBView.BufferLocation = UIVB->GetGPUVirtualAddress();
    VBView.SizeInBytes = UINT(UIVertSize);
    VBView.StrideInBytes = sizeof(ui_vertex);
    CmdList->IASetVertexBuffers(0, 1, &VBView);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->DrawInstanced(UIVertCount, 1, 0, 0);
}