#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu200", scriptLoadByNumber);

INCLUDE_ASM("asm/main/nonmatchings/tu200", scriptReadFile);

void func_002A93A8(int value);

/* Frees the buffer in *slot (allocated by scriptReadFile) and clears the slot. */
void scriptFreeBuffer(int *slot) {
    func_002A93A8(*slot);
    *slot = 0;
}
