#include <windows.h>

int CALLBACK 
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{

    char Name[256];
    DWORD Size = sizeof(Name);
    GetComputerNameA(Name, &Size);
    MessageBoxA(0, Name, "Your PC", MB_OK);

    MessageBoxA (0, "This is Handmade Hero.", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
    return 0;
}