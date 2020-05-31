#include "sdf_engine.cpp"

#include <string.h>
#include <stdio.h>
#include "ShellScalingAPI.h"
#include <windows.h>
#include <windowsx.h>

global bool gAppIsDone;
global int gClientWidth = 1280;
global int gClientHeight = 720;
global input gInput;

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

global WINDOWPLACEMENT GlobalWindowPosition;
internal void
Win32ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(Window, GWL_STYLE,
                          Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
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
            
            if (KeyWasDown != KeyIsDown)
            {
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
                    
                    if (AltIsDown && WParam == VK_RETURN)
                    {
                        Win32ToggleFullscreen(Window);
                    }
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
        
        case WM_ACTIVATE:
        {
            bool IsDeactivated = LOWORD(WParam) == WA_INACTIVE;
            
            if (IsDeactivated)
            {
                for (int KeyI = 0; KeyI < ARRAY_COUNT(gInput.Keys); ++KeyI)
                {
                    gInput.Keys[KeyI] = 0;
                }
                
                gInput.MouseDown = false;
            }
        } break;
        
        case WM_MOUSELEAVE:
        {
            gInput.MouseDown = false;
        } break;
        
        case WM_SIZE:
        {
            gClientWidth = LOWORD(LParam);
            gClientHeight = HIWORD(LParam);
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal FILETIME 
GetLastWriteTime(char *FilePath)
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

struct file_tracker
{
    FILETIME LastWriteTime;
    char Path[256];
};

global file_tracker gFileTrackers[1000];
global int gFileTrackerCount;

internal void
TrackAllCodeFiles()
{
    WIN32_FIND_DATA Entry;
    HANDLE find = FindFirstFile("../code/*.*", &Entry);
    if (find != INVALID_HANDLE_VALUE)
    {
        do 
        {
            printf("Name: %s ", Entry.cFileName);
            
            if (Entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
            {
                // directory
            } 
            else 
            {
                size_t FilenameLen = strlen(Entry.cFileName);
                if (FilenameLen >= 5)
                {
                    char *Suffix = Entry.cFileName + FilenameLen - 5;
                    if (strncmp(Suffix, ".hlsl", 5) == 0)
                    {
                        if (gFileTrackerCount == ARRAY_COUNT(gFileTrackers))
                        {
                            Win32Panic("Internal Error: File write-time tracker reached maximum count of 1000");
                        }
                        
                        int CurrTrackerIndex = gFileTrackerCount++;
                        file_tracker *Tracker = gFileTrackers + CurrTrackerIndex;
                        
                        snprintf(Tracker->Path, sizeof(Tracker->Path),
                                 "../code/%s", Entry.cFileName);
                        Tracker->LastWriteTime = GetLastWriteTime(Tracker->Path);
                    }
                }
            }
            
        } while (FindNextFile(find, &Entry));
        
        FindClose(find);
    }
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
        WindowRect.right = gClientWidth;
        WindowRect.bottom = gClientHeight;
        
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
        
        TrackAllCodeFiles();
        
        while (!gAppIsDone)
        {
            TRACKMOUSEEVENT EventToTrack = {};
            EventToTrack.cbSize = sizeof(EventToTrack);
            EventToTrack.dwFlags = TME_LEAVE;
            EventToTrack.hwndTrack = Window;
            EventToTrack.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&EventToTrack);
            
            gInput.MousedP = {};
            
            MSG Message;
            while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            
            b32 NeedsReload = false;
            for (int TrackerI = 0; TrackerI < gFileTrackerCount; ++TrackerI)
            {
                file_tracker *Tracker = gFileTrackers + TrackerI;
                FILETIME CurrWriteTime = GetLastWriteTime(Tracker->Path);
                if (CompareFileTime(&CurrWriteTime, &Tracker->LastWriteTime) != 0)
                {
                    NeedsReload = true;
                    Tracker->LastWriteTime = CurrWriteTime;
                }
            }
            
            Engine.UpdateAndRender(Window, gClientWidth, gClientHeight,
                                   &gInput, NeedsReload);
            
            Sleep(2);
        }
    }
    else
    {
        Win32Panic("Failed to register window class");
    }
    
    return 0;
}
