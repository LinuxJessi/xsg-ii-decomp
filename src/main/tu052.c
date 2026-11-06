#include "common.h"

/*
 * Per uwamonoRecords[index] (called from DispMapObject): three Get_MiddlePoint pairs write midpoints to the record's
 * object (the pointer at +0x330) at +0x190 and +0x200; short counters at +0x222..+0x244 pace them, and the float at
 * +0xCC is blended over a frame count.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu052", uwamonoUpdateMidpoints);

/*
 * Per uwamonoRecords[index] (called from DispMapObject before uwamonoUpdateMidpoints): copies the object's midpoints back into
 * the record (+0xF0, +0x1B0, +0x1E0, +0xCC) and stores the frame count at +0x248. A debug line goes out when the flags
 * word returned through the stack has 0x500 set.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu052", func_001DEDD8);

/* Returns 0. No caller inside this unit. */
int func_001DF088(void) {
    return 0;
}
