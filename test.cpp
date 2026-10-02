#include <Windows.h>
#include <stdio.h>

// Function definition
void foo(void)
{
	const char* Foo = "This is the first thing we have actually printed.\n";
	OutputDebugStringA(
		"Line 0\\Line 1\n");
	printf("djwjdiwjiefjifjrfreifjirjferif\n");
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine, 
	int nCmdShow)
{
	/*
	char SmallS; // 00000000 slots for 1 and 0, 8 bits, 1 byte
	// thus, char being an 8bit type, we have 2^8 (256) possible values (-128 - +127)

	char unsigned SmallU; // Same as signed, but do not represent negative values (0 - 256)

	short MediumS; // 16 bits, 2 bytes - 65536 values
	
	int LargeS; // 32 bits 4 bytes - ~4 billion values


	*/



	char unsigned Test;
	char unsigned* TestPointer = &Test;


	Test = 255;
	Test = Test + 1;
	foo();
	return 0;
}

 