#include "common.h"
#include "main/tu026.h"

INCLUDE_ASM("asm/main/nonmatchings/test_129", testMenuRun);

int xglDebFileRead(int file, int address);
int xglDebFileSize(int file);
extern int D_0068C788;
extern int D_0068C790;

/*
 * Reads a debug file into the next 16-byte aligned block of the test heap
 * cursor (*bufferCursor) and returns its address, or 0 for an empty file.
 * The NULL check is the assert on pData at ../test.euc.c line 95.
 */
int testLoadFile(int *bufferCursor, int fileHandle) {
    int buffer;
    int size;

    buffer = 0;
    size = xglDebFileSize(fileHandle);
    if (size > 0) {
        buffer = *bufferCursor;
        *bufferCursor = (buffer + size + 0xF) & ~0xF;
        if (buffer == 0) {
            assertFailed(&D_0068C788, &D_0068C790, 95);
        }
        xglDebFileRead(fileHandle, buffer);
    }
    return buffer;
}

INCLUDE_ASM("asm/main/nonmatchings/test_129", func_0023EBB8);

/* Bump pointer of the test heap. */
extern int testHeapCursor;

/* Sets the test heap cursor. */
void testHeapSetCursor(int newCursor) {
    testHeapCursor = newCursor;
}

/* Bump allocation: returns the next 16-byte aligned address and moves the cursor past size bytes. */
int testHeapAlloc(int size) {
    int buffer;
    int cursorAfter;

    buffer = (testHeapCursor + 0xF) & ~0xF;
    cursorAfter = buffer + size;
    testHeapCursor = cursorAfter;
    return buffer;
}
