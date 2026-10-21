#include <windows.h>

#define internal_function static
#define local_persist static
#define global_variable static

//TODO: remove al globals

global_variable bool Running; // global variable turned on or off if our program is running

global_variable BITMAPINFO BitmapInfo; // the request
global_variable void* BitmapMemory; // the memory (pixels themselves)
global_variable HBITMAP BitmapHandle; // windows handle
global_variable HDC BitmapDeviceContext; // draws to a bitmap

internal_function void Win32ResizeDIBSection(int Width, int Height)
{
    if (BitmapHandle)
    {
        DeleteObject(BitmapHandle); // Delete the old bitmap if there is one
    }

    if (!BitmapDeviceContext)
    {
        BitmapDeviceContext = CreateCompatibleDC(0); // make sure a memory DC exists
    }

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapHandle = CreateDIBSection(
        BitmapDeviceContext, &BitmapInfo,
        DIB_RGB_COLORS,
        &BitmapMemory,
        0, 0);
}

internal_function void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(DeviceContext,
        X, Y, Width, Height,
        X, Y, Width, Height,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) // windows call each message, we can define the behavior for each as well
{
    LRESULT Result = 0;
    switch (Message) {
    case WM_SIZE: {  // if our window change size, it reads the new drawable area, and rebuilds our pixel buffer
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        int Width = ClientRect.right - ClientRect.left;
        int Height = ClientRect.bottom - ClientRect.top;
        Win32ResizeDIBSection(Width, Height); // resize buffer function
        OutputDebugStringA("WM_SIZE\n");
    }break;
    case WM_DESTROY: {
        Running = false;
        OutputDebugStringA("WM_DESTROY\n");
    }break;
    case WM_CLOSE: {
        Running = false;
        OutputDebugStringA("WM_CLOSE\n");
    }break;
    case WM_ACTIVATEAPP: {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;
    case WM_PAINT: { // if our window needs to be redrawed this runs, basically showing the buffer on WM_SIZE
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

        Win32UpdateWindow(DeviceContext, X, Y, Width, Height);

        EndPaint(Window, &Paint);
    }break;
    default: {
        Result = DefWindowProc(Window, Message, WParam, LParam);
    }break;
    }
    return Result;
}

int CALLBACK                            // MAIN FUNCTION
WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    WNDCLASS WindowClass{}; // creating a windowclass struct

    WindowClass.lpfnWndProc = Win32MainWindowCallback; // who deals with the events, windows itself calls this function for messages
    WindowClass.hInstance = Instance; 
    WindowClass.lpszClassName = L"HandnameHeroWindowClass"; 

    if (RegisterClass(&WindowClass)) // IF the registered windowclass doesnt fail
    {
        HWND WindowHandle = CreateWindowEx( // we create a window 
            0,
            WindowClass.lpszClassName,
            L"Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );
        if (WindowHandle) // IF the windowhandle doesnt return fail
        {
            Running = true; 
            while (Running) // we start our main messages loop
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
        }
    }
    else
    {
    }

    return 0;
}


// for day 4 - animate backbuffer