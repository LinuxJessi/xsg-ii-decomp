#include "common.h"
#include "main/tu082.h"

void func_A9CDB0(void *module, void *nativeTable);
extern unsigned char menuNativeTable[];
extern unsigned char D_0069D3F0[];

/* Registers the menu native table menuNativeTable (ptInit ... umnReturn) under the module descriptor D_0069D3F0 ("menu"). */
void menuInit(void) {
    func_A9CDB0(D_0069D3F0, menuNativeTable);
}

/* Empty in the original: a no-op, called by func_001D6070. */
void func_001FCBF8(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/tu082", func_001FCC00);
