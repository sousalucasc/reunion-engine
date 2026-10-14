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

struct projectile {
	char unsigned isThisOnFire = 0;
	int Damage = 0;
	int ParticlesPerSecond = 0;
	short HowManyCooks = 0;

	// 1 + 4 + 4 + 2 bytes, = should theoretically be 11 bytes
};


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

	int x = 5;
	int y = 10;
	x = (1 << 4) | (1 << 8);   
	y = (1 << 8) | (1 << 16);  
	x = x&y;  
	
	return 0;
	}
// DAY 6




// DAY 5


// Code lives in the executable file, the code is encoded in a format that is defined by intel and amd
// Decoded in binary telling what instructions the CPU needs to use, in order
// 
// CODE -> OBJ'S -> EXE
// CODING -> COMPILING -> LINKING
// basically the code we write turns into an obj file, that will be linked to other obj libs
// and everything will be a executable file	

// there will be always some linking needed to do outside of the code, because our program
// will run inside windows, and they are not windows itself!

// SUPPOSE WE HAVE A CODE MEMORY BLOCK:
// XXXXXXXXLLLXXXXXXLLLXXXXXXXXLLXXXXXXXXXXXXXXXXXXXXLL
// the X would be our program itself, the L would be pointers to our actual windows code!

// # OPERATORS
// just like in math
// int x;int y;int z = x*y + y*3 - 3/y

// # BITWISE OPERATORS
// int x = 0x0000000a (10)
// x = x << 4 (0x000000a0) (160)
// x = x >> 4 (0x0000000a) (10)

// # ADVANCED BITWISE OPERATORS
//
// int x = 0;         (0x00000000)
// x = x | (1 << 4);  (0x00000010)
// x = x | (1 << 8);  (0x00000110)
// x = x & (1 << 4);  (0x00000010)
// 
// | - LIGA BITS
// & - FILTRA
// 
// NEGATION
// int x = 0x00110110
// x = ~x (x would be 0xffeefeef)
// TURNS INTO OPPOSITE
// 
// 
// int x = 5;
// int y = 10;
//
//  EXCLUSIVE OR
// x = (1 << 4) | (1 << 8);   (0x00000110)
// y = (1 << 8) | (1 << 16);  (0x00010100)
// x = x^y;                   (0x00010010)  remove the bitset both have
// 
// OR
// x = (1 << 4) | (1 << 8);   (0x00000110)
// y = (1 << 8) | (1 << 16);  (0x00010100)
// x = x&y;                   (0x00000100) just leaves the bitset both have
//
// # BOOLEANS (LOGICAL OPERATORS)
// anything that isnt a 0 is true, as simple as that.
//
// Q&A
//



// DAY 4


// learned endianess, how values are represented in the block of memory
// 500 as unsigned short is represented as 244 and 1 (each number for each byte, since short are 2 bytes)

// learned how structs reserve memory, they might reserve extra memory for small value types, on the example above, the struct
// needed 11 bytes. but it reserved 16, 4 bytes for every variable

// learned hexadecimal
// THE ONLY REASON TO USE HEXADECIMAL IS: IT ALIGNS PERFECTLY WITH BINARY	
// 0xA = 10
// 0xAA = 16*10 + 10 = 170
// 0xAAA = 16*16*10 + 16*10 + 10 = 2730

// learned casting
// projectile Test; unsigned short *MrPointer = (unsigned short *)&Test
// essentially treating the whole object as an unsigned short, converted type.

// learned arrays
// projectile Projectiles[40], essentially creating 40 projectiles
// projectile *ProjectilePointer = Projectiles; (projectiles array is essentially a pointer to the first element!!)
// 
// Great thing about pointers is that we can use addition to access the array values too
// 
// Projectiles[30].Damage = 50;
//    would perform the same operation as:
// (ProjectilePointer + 30)->Damage = 50;
//    we can also be specially dumb and do all the work by ourselves:
// ((projectile *)((char *)ProjectilePointer + 30*sizeof(projectile)))->Damage = 100;
// (essentialy, casting ProjectilePointer as a 1byte value, then go 30 (sizeof projectile) times to go to the element
// then casting it back as a projectile pointer, then assigning it the damage value to 100

// learned how to access pointers
// projectile Test;
// Test.Damage = 10;
// 
//  same fkin thing as
// 
// projectile *Test
// Test->Damage = 10;