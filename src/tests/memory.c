#include "core.h"
#include "platform.h"
#include "tests.h"

cs_bool Tests_Memory(void) {
	Tests_NewTask("Try alloc memory");
	unsigned char *mem, *mem2;
	Tests_Assert((mem = Memory_TryAlloc(1000, 1)) != NULL, "allocate first memory block");
	Tests_Assert((mem2 = Memory_TryAlloc(1000, 1)) != NULL, "allocate second memory block");

	Tests_NewTask("Check memory block size");
	Tests_Assert(Memory_GetSize(mem) >= 1000, "check first memory block size");
	Tests_Assert(Memory_GetSize(mem2) >= 1000, "check second memory block size");

	Tests_NewTask("Try to realloc memory block");
	Tests_Assert((mem = Memory_Realloc(mem, 100)) != NULL, "reallocate first memory block");
	Tests_Assert((mem2 = Memory_Realloc(mem2, 100)) != NULL, "reallocate second memory block");

	Tests_NewTask("Check reallocated memory block size");
	Tests_Assert(Memory_GetSize(mem) >= 100, "check first memory block size");
	Tests_Assert(Memory_GetSize(mem2) >= 100, "check second memory block size");

	Tests_NewTask("Memory filling");
	Memory_Fill(mem, 100, 0x66);
	for(int i = 0; i < 100; i++) Tests_Assert(mem[i] == 0x66, "test filled memory");

	Tests_NewTask("Memory copying");
	Memory_Copy(mem2, mem, 50);
	for(int i = 0; i < 100; i++) Tests_Assert(mem2[i] == (i < 50 ? mem[i] : 0x00), "compare copied memory");

	Memory_Free(mem);
	Memory_Free(mem2);
	return true;
}
