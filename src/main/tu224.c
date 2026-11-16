#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu224", timecodeStartAtHms);

INCLUDE_ASM("asm/main/nonmatchings/tu224", timecodeSetHmsf);

void timecodeSetMode(void *timecode, int mode);

void timecodeSetModeForward(void *timecode, int mode) {
    timecodeSetMode(timecode, mode);
}

INCLUDE_ASM("asm/main/nonmatchings/tu224", timecodeSetFramePairFromHms);
