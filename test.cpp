#include <Windows.h>

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	OutputDebugStringA("Hello World\n");
	return 0;
}

void foo(void)
{
}