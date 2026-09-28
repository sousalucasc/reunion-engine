#include <Windows.h>

void foo(void) // Function definition signature or prototype
{
	OutputDebugStringA("Hello World\n");
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine, 
	int nCmdShow)
{
	char SmallS; // 00000000 slots for 1 and 0, 8 bits, 1 byte
	// thus, char being an 8bit type, we have 2^8 (256) possible values (-128 - +127)

	char unsigned SmallU; // Same as signed, but do not represent negative values (0 - 256)

	short MediumS; // 16 bits, 2 bytes - 65536 values
	
	int LargeS; // 32 bits 4 bytes - ~4 billion values

	foo();
	return 0;
}

 