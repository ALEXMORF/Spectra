#include "sdf_engine.cpp"

#include <stdio.h>
#include "ShellScalingAPI.h"
#include <windows.h>

global bool gAppIsDone;

internal void
Win32Panic(char *Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    
    char Buf[256];
    vsnprintf(Buf, sizeof(Buf), Fmt, Args);
    va_end(Args);
    
    MessageBoxA(0, Buf, "Panic", MB_OK|MB_ICONERROR);
    
    ExitProcess(1);
}

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
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
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
    WindowClass.lpszClassName = "sdf engine window class";
    
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
                                      "SDF Engine",
                                      WindowStyle,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      WindowWidth,
                                      WindowHeight,
                                      0, 0, Instance, 0);
        
        engine Engine = {};
        
        while (!gAppIsDone)
        {
            MSG Message;
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            
            Engine.UpdateAndRender(Window);
            
            Sleep(2);
        }
    }
    else
    {
        Win32Panic("Failed to register window class");
    }
    
    return 0;
}
