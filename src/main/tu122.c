#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu122", func_00233D78);

/* Draws the entry list: each 16-byte record dispatches by type to a draw helper. */
INCLUDE_ASM("asm/main/nonmatchings/tu122", func_00234500);

/* Result screen: RESULT and ClearTime labels, clear time split into minutes and seconds. */
INCLUDE_ASM("asm/main/nonmatchings/tu122", func_002348C8);

void func_002348C8(void);

/* Tail call to the result screen (func_002348C8). */
void func_00234C10(void) {
    func_002348C8();
}

/* Builds the 16-byte entry records at D_0069D90C from the table at a0 (count at +0x4C). */
INCLUDE_ASM("asm/main/nonmatchings/tu122", func_00234C28);

/* Scene loop: clears the environment, builds the entries, runs frames until done. */
INCLUDE_ASM("asm/main/nonmatchings/tu122", func_00234D78);
