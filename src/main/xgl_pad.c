#include "common.h"

/* Returns at once with no effect. Only caller: InitializeSystem. */
void func_00199428(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_pad", xglPadInitial);

/*
 * Auto-repeat setup of one port (-1: both), stored at PadData + port * 0x80:
 * +0x48 delay, +0x49 interval, +0x4a mask, +0x4c and +0x4d cleared. Called by
 * xglPadInitial with mask 0xF00C, delay 8, interval 1.
 */
INCLUDE_ASM("asm/main/nonmatchings/xgl_pad", xglPadSetRepeat);

INCLUDE_ASM("asm/main/nonmatchings/xgl_pad", xglPadRead);
