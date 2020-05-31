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
    int FontHeight = 20;
    
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
    System.FontAtlas = InitTexture2D(Context->Device, 
                                     AtlasWidth, AtlasHeight,
                                     DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_NONE,
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context->Upload(&System.FontAtlas, AtlasData);
    free(AtlasData);
    
    return System;
}