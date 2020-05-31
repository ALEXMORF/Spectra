#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct ui_system
{
    texture FontAtlas;
    int BeginCharI;
    int EndCharI;
    int CharXCount;
    int CharYCount;
    int FontWidth;
    int FontHeight;
    
    pso RasterizeTextPSO;
};