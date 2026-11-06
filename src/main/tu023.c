#include "common.h"

/* Interrupt handlers of main/tu023. Each handler takes an index, maps it through xglIntcSlotMap to a
 * 16-byte record in xglIntcEventCounts and adds 1 to the word at its start. Nine of the ten handlers are the
 * same code; the table of handler addresses starts at 0x00644FC4 with 16-byte entries. */

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4768);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A47A8);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A47E8);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4828);

/* Timer accumulator. It reads the EE timer 1 count (0x10000800). When intrTimerActive is set it adds the
 * ticks since the start count to intrTimerAccum and clears the flag; otherwise it sets the flag and
 * records the start count. The same handler also bumps its counter record. */
INCLUDE_ASM("asm/main/nonmatchings/tu023", intrTimerAccumulate);

/* Timer accumulator: intrTimerActive active flag, intrTimerStart start count, intrTimerAccum ticks total. */
extern int intrTimerActive;
extern int intrTimerStart;
extern int intrTimerAccum;

/* Clears the timer accumulator. */
void intrTimerReset(void)
{
    intrTimerActive = 0;
    intrTimerStart = 0;
    intrTimerAccum = 0;
}

/* Returns the accumulated timer ticks. */
int intrTimerGetAccum(void)
{
    return intrTimerAccum;
}

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4918);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4958);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4998);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A49D8);

INCLUDE_ASM("asm/main/nonmatchings/tu023", func_001A4A18);
