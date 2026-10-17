#include <windows.h>
LRESULT CALLBACK // LRESULT is a long ptr, callback is __stdcall, useless in 64bit, keeping for the sake of windows compatibility
MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) // Which window, What event happened, and extra parameters for the messages
{
    LRESULT Result = 0; // if you handle the windows messages, this should be set to zero
    switch (Message) {
        // if the user resizes the window
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        // when windows deletes our window
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        }break;
        // when the user clicks on the X on the window or alt f4
        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
            //DestroyWindow(Window); // makes the window close with the X or alt f4
        }break;
        // when the user clicks in us (window as active)
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        }break;
        // when windows decided that the window need to be repainted
        case WM_PAINT:
        {
            PAINTSTRUCT Paint; // struct containing information about repainting
            HDC DeviceContext = BeginPaint(Window, &Paint); // fills paint in the area it needs to repaint (will paint when resizing!)
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top; // stores the values of the actual retangle
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top; // takes the exact area by subtracting the rectangles
            PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS); // drawing function, it fills the entire screen because now we have the exact area that isnt painted when resized
            for (int i = 50; i < 250; i++)
            {
                SetPixel(DeviceContext, i, 100, RGB(255, 0, 0));
            }
            EndPaint(Window, &Paint);
        }
        // if none of this conditions run
        default:
        {
            //OutputDebugStringA("default\n")
            Result = DefWindowProc(Window,Message,WParam,LParam);
        }break;
    }
    return Result;
}


int CALLBACK 
WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    WNDCLASS WindowClass{}; // initializes the WNDCLASS struct, with all default values to zero

    //WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Allocates unique device context for each window in the class, and it redraws while resizing
    WindowClass.lpfnWndProc = MainWindowCallback; // IMPORTANT!, assigns the return value of the callback functions to windowclass!
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = L"HandnameHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
            0,                               // parameters for misc, drag & drop, etc etc
            WindowClass.lpszClassName,       // actual window class
            L"Handmade Hero",                // actual window name
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,// The window is an overlapped window (its also an aggregate to a bunch of other values but this one fits perfectly), And its visible
            CW_USEDEFAULT,  // X pos             
            CW_USEDEFAULT,  // Y pos
            CW_USEDEFAULT,  // width
            CW_USEDEFAULT,  // heigth
            0, // parent windows (none)
            0, // menu (none)
            Instance, // my hInstance
            0 // lpParam
            );
        // if the function fails, it will return a null value, so we check its condition
        if (WindowHandle) 
        {
            MSG Message;
            for (;;) 
            {
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
            //TODO logging
        }
    }
    else 
    {
        //TODO logging
    }


    return 0;
}


// DAY 1

// learned how to create a simple message box in windows