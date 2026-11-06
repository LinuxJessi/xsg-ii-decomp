#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu224", timecodeStartAtHms);

INCLUDE_ASM("asm/main/nonmatchings/tu224", timecodeSetHmsf);

void timecodeSetMode(void *timecode, int mode);

void func_0027DBD0(void *timecode, int mode) {
    timecodeSetMode(timecode, mode);
}

INCLUDE_ASM("asm/main/nonmatchings/tu224", func_0027DBE8);
