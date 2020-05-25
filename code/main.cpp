#include "sdf_engine.cpp"

#include <stdio.h>
#include "ShellScalingAPI.h"
#include <windows.h>
#include <windowsx.h>

global bool gAppIsDone;

internal void
Win32MessageBox(char *Title, UINT Type, char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char Buf[2048];
    vsnprintf(Buf, sizeof(Buf), Fmt, Args);
    va_end(Args);
    
    MessageBoxA(0, Buf, Title, Type);
}

internal void
Win32Panic(char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char Buf[2048];
    vsnprintf(Buf, sizeof(Buf), Fmt, Args);
    va_end(Args);
    
    MessageBoxA(0, Buf, "Panic", MB_OK|MB_ICONERROR);
    
    ExitProcess(1);
}

global input gInput;

LRESULT CALLBACK 
Win32WindowCallback(HWND Window, UINT Message, 
                    WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_CLOSE:
        case WM_QUIT:
        {
            gAppIsDone = true;
        } break;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            b32 KeyIsDown = (LParam & (1 << 31)) == 0;
            b32 KeyWasDown = (LParam & (1 << 30)) != 0;
            b32 AltIsDown = (LParam & (1 << 29)) != 0;
            
            if (KeyIsDown)
            {
                gInput.Keys[WParam] += 1;
                
                // prevent overflow
                if (gInput.Keys[WParam] == 1000)
                {
                    gInput.Keys[WParam] = 1000;
                }
            }
            else
            {
                gInput.Keys[WParam] = 0;
            }
            
            if (KeyIsDown)
            {
                if (WParam == VK_ESCAPE)
                {
                    gAppIsDone = true;
                }
                
                if (AltIsDown && WParam == VK_F4)
                {
                    gAppIsDone = true;
                }
            }
        } break;
        
        case WM_MOUSEMOVE:
        {
            v2i OldMouseP = gInput.MouseP;
            
            gInput.MouseP.X = GET_X_LPARAM(LParam);
            gInput.MouseP.Y = GET_Y_LPARAM(LParam); 
            
            if (gInput.MouseDataIsInitialized)
            {
                gInput.MousedP = gInput.MouseP - OldMouseP;
            }
            else
            {
                gInput.MouseDataIsInitialized = true;
            }
        } break;
        
        case WM_LBUTTONDOWN:
        {
            gInput.MouseDown = true;
        } break;
        
        case WM_LBUTTONUP:
        {
            gInput.MouseDown = false;
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

FILETIME GetLastWriteTime(char *FilePath)
{
    FILETIME Result = {};
    
    HANDLE SceneFile = CreateFileA(FilePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   0, OPEN_EXISTING, 0, 0);
    if (SceneFile != INVALID_HANDLE_VALUE)
    {
        GetFileTime(SceneFile, 0, 0, &Result);
        CloseHandle(SceneFile);
    }
    
    return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance, 
        HINSTANCE PrevInstance, 
        LPSTR pCmdLine, 
        int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursorA(0, IDC_ARROW);
    WindowClass.lpszClassName = "Spectra window class";
    
    if (RegisterClassA(&WindowClass))
    {
        // first input client rect
        RECT WindowRect = {};
        WindowRect.left = 0;
        WindowRect.top = 0;
        WindowRect.right = WIDTH;
        WindowRect.bottom = HEIGHT;
        
        DWORD WindowStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
        
        // adjust to window rect
        AdjustWindowRect(&WindowRect, WindowStyle, 0);
        int WindowWidth = WindowRect.right - WindowRect.left;
        int WindowHeight = WindowRect.bottom - WindowRect.top;
        
        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "Spectra",
                                      WindowStyle,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      WindowWidth,
                                      WindowHeight,
                                      0, 0, Instance, 0);
        
        engine Engine = {};
        
        char *HotCodeFilename = "../code/scene.hlsl";
        FILETIME SceneFileLastWriteTime = GetLastWriteTime(HotCodeFilename);
        
        while (!gAppIsDone)
        {
            gInput.MousedP = {};
            
            MSG Message;
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            
            FILETIME LastWriteTime = GetLastWriteTime(HotCodeFilename);
            int CmpRes = CompareFileTime(&SceneFileLastWriteTime, &LastWriteTime);
            b32 NeedsReload = CmpRes != 0;
            if (NeedsReload) SceneFileLastWriteTime = LastWriteTime;
            
            Engine.UpdateAndRender(Window, &gInput, NeedsReload);
            
            Sleep(2);
        }
    }
    else
    {
        Win32Panic("Failed to register window class");
    }
    
    return 0;
}
