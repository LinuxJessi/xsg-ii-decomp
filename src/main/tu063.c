#include "common.h"
#include "main/elevator.h"
#include "main/tu022.h"
#include "main/tu063.h"

void tyaDisplaySetting(int);
void tyaDrawGauge(int);

/* Draws the gauge, resets the elevator records, then runs the display
 * adjustment screen into the framebuffer at 0x01000000. */
void tyaRunDisplaySetting(void) {
    tyaDrawGauge(0);
    elvReset();
    tyaDisplaySetting(0x01000000);
}

/* Debug menu loop ("SELECT" title, clock line); the selected row runs
 * func_51 in the original. Reached from the menu rows of the yajima test. */
INCLUDE_ASM("asm/main/nonmatchings/tu063", YajimaTest);


/* Channel level 0x5A (bgm fade). Byte-identical to func_001EDC40. */
void func_001EDC20(int index) {
    soundSlotSetVolume(index, 0x5A, 0);
}

/* Channel level 0x5A; same body as func_001EDC20. */
void func_001EDC40(int index) {
    soundSlotSetVolume(index, 0x5A, 0);
}

/* Channel level 0x7A; same body as streamSetLevel. */
void func_001EDC60(int index) {
    soundSlotSetVolume(index, 0x7A, 0);
}

/* Channel level 0x7A (stream play). Byte-identical to func_001EDC60. */
int streamSetLevel(int index) {
    return soundSlotSetVolume(index, 0x7A, 0);
}
