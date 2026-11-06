/*
 * OV01 original TU 40: 0x00a9b0f8..0x00a9b120 (3 functions)
 */
#include "common.h"

/* Returns at once with no effect. Caller: ov01SceneInit. */
void func_00A9B0F8(void) {
}

/*
 * Returns the word at 0x002B5DBC (a main-ELF global, name unknown).
 * ov01SceneRelease passes it to sbThreadsRelease as a key.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu040", func_00A9B100);

/*
 * Returns the word at 0x002B5E48 (a main-ELF global, name unknown).
 * func_00A9A6A0 passes it to func_00A9CEE8 with the value of func_00A9B100.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu040", func_00A9B110);
