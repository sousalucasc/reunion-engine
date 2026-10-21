#include <windows.h>
#include <stdint.h>

#define internal_function static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

//TODO: remove al globals

global_variable bool Running; // global variable turned on or off if our program is running

global_variable BITMAPINFO BitmapInfo; // the request
global_variable void* BitmapMemory; // the memory (pixels themselves)
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal_function void RenderWeirdGradient(int XOffset, int YOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;
    int Pitch = Width * BytesPerPixel;
    uint8* Row = (uint8*)BitmapMemory;

    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint8* Pixel = (uint8*)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            //blue channel
            *Pixel = (uint8)(X + XOffset);
            ++Pixel;
            //green channel
            *Pixel = (uint8)(Y + YOffset);
            ++Pixel;
            //red channel
            *Pixel = 0;
            ++Pixel;
            //padding byte
            *Pixel = 0;
            ++Pixel;
        }

        Row += Pitch;
    }
}

internal_function void Win32ResizeDIBSection(int Width, int Height)
{
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory,0,MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    int BitmapMemorySize = BitmapWidth * BitmapHeight * BytesPerPixel;
    BitmapMemory = VirtualAlloc(0,BitmapMemorySize,MEM_COMMIT,PAGE_READWRITE);


}

internal_function void Win32UpdateWindow(HDC DeviceContext,RECT *ClientRect, int X, int Y, int Width, int Height)
{
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;

    StretchDIBits(DeviceContext,
        0, 0, BitmapWidth, BitmapHeight,
        X, Y, WindowWidth, WindowHeight,
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

        RECT ClientRect;
        GetClientRect(Window, &ClientRect);

        Win32UpdateWindow(DeviceContext,&ClientRect, X, Y, Width, Height);

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
        HWND Window = CreateWindowEx( // we create a window 
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
        if (Window) // IF the windowhandle doesnt return fail
        {
            int YOffset = 0;
            int XOffset = 0;
            Running = true; 
            while (Running) // we start our main messages loop
            {
                
                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderWeirdGradient(YOffset, XOffset);
                HDC DeviceContext = GetDC(Window);

                RECT ClientRect;
                GetClientRect(Window, &ClientRect);

                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;
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