#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/main/nonmatchings/tu197", MMathMatrixBetweenVectors);

extern void *memset(void *destination, int value, unsigned int count);
extern short MSeObjReady;
extern int MSeObjCount;
extern int MSeObjPool;
extern int MSeMode1Hook;
extern int MSeMode2Hook;

/* Initialises the MSe object pool: 127 slots of 0x800 bytes at 0x01FB0000. */
void MSeObjInit(void) {
    MSeObjPool = 0x01FB0000;
    MSeObjReady = 0;
    MSeObjCount = 0;
    MSeMode1Hook = 0;
    MSeMode2Hook = 0;
    memset((void *)0x01FB0000, 0, 0x3F800);
    MSeObjReady = 1;
}

/* Shutdown: clears the ready flag, the live count and the pool pointer. */
void MSeObjShutdown(void) {
    MSeObjReady = 0;
    MSeObjCount = 0;
    MSeObjPool = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu197", MSeObjProc);

INCLUDE_ASM("asm/main/nonmatchings/tu197", MSeObjAlloc);

/* Marks the slot whose flags word is *flags for release: MSeObjProc mode 3 clears slots carrying 0x100. */
void MSeObjRequestFree(unsigned short *flags) {
    *flags |= 0x100;
}

/* Sets the callback that MSeObjProc calls with no arguments after its mode 1 pass (MSeMode1Hook). */
void MSeSetMode1Hook(int hook) {
    MSeMode1Hook = hook;
}

/* Sets the callback that MSeObjProc calls with no arguments after its mode 2 pass (MSeMode2Hook). */
void MSeSetMode2Hook(int hook) {
    MSeMode2Hook = hook;
}
