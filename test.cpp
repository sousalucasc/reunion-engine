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

	projectile Test;

	int SizeOfChar = sizeof(char unsigned);
	int SizeOfInt = sizeof(int);
	int SizeOfProjectile = sizeof(projectile);
	int SizeOfTest = sizeof(Test);

	
	
	Test.isThisOnFire = 1;
	Test.Damage = 1;
	Test.ParticlesPerSecond = 1;
	Test.HowManyCooks = 1;
	
	return 0;
	}

// learned endianess, how values are represented in the block of memory
// 500 as unsigned short is represented as 244 and 1 (each number for each byte, since short are 2 bytes)

// learned how structs reserve memory, they might reserve extra memory for small value types, on the example above, the struct
// needed 11 bytes. but it reserved 16, 4 bytes for every variable

// learned hexadecimal
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