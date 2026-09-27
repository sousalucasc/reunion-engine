#include <Windows.h>

void foo(void); // Function declaration

void foo(void) // Function definition signature or prototype
{
	OutputDebugStringA("Hello World\n");
}

int CALLBACK WinMain(
	HINSTANCE hInstance, // Handle that we can use when talking to windows to refer as our running program
	HINSTANCE hPrevInstance, // Legacy thingy
	LPSTR lpCmdLine, // What got sent to us when we run the program through cli (or launch arguments like -dx12 on steam or whatever)
	int nCmdShow) // Number that corresponds to a legacy option on properties of exe (run normal window/minimized/maximized)
{
	foo();
	return 0;
}

