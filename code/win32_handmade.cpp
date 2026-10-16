#include <windows.h>
LRESULT CALLBACK
MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message) {
        // if the user resizes the window
        case WM_SIZE:
        {

        }
            break;
        // when windows deletes our window
        case WM_DESTROY:
        {

        }
            break;
        // when the user clicks on the X on the window
        case WM_CLOSE:
        {

        }
            break;
        // when the user clicks in us (window as active)
        case WM_ACTIVATEAPP:
        {

        }
            break;
        // if none of this conditions run
        default:
        {

        }
            break;
    }

}


int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    WNDCLASS WindowClass{}; // initializes the WNDCLASS struct, with all default values to zero
  
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; // Allocates unique device context for each window in the class, and it redraws while resizing
    WindowClass.lpfnWndProc = ;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandnameHeroWindowClass";
 



    return 0;
}


// DAY 1

// learned how to create a simple message box in windows