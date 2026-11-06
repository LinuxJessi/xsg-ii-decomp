#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu089", uwamonoGetSectorEnds);

INCLUDE_ASM("asm/main/nonmatchings/tu089", uwamonoCheckCollision);

INCLUDE_ASM("asm/main/nonmatchings/tu089", func_00208B28);

/* Stores into the word at D_002B60C4; what that word means is not known yet. */
extern int D_002B60C4[];

void func_00208B50(int value) {
    D_002B60C4[0] = value;
}

/*
 * The byte fade block at gameWork. vibrationStartFade starts a fade from currentValue
 * towards targetValue over framesTotal steps; func_00208B80 steps it and writes
 * currentValue to PadData[0].+0x51, with PadData[0].+0x50 as the running flag.
 */
typedef struct ByteFade {
    unsigned char unmodeled_0000[0x332];
    unsigned char targetValue;          /* +0x332 */
    unsigned char currentValue;         /* +0x333 */
    unsigned char unmodeled_0334[0x4];  /* +0x334: hold countdown, read by func_00208B80 */
    int framesLeft;                     /* +0x338: steps left in the fade */
    int framesTotal;                    /* +0x33C: steps the fade was started with */
} ByteFade;

extern ByteFade gameWork;

void vibrationStartFade(unsigned char targetValue, unsigned char currentValue, int framesTotal) {
    gameWork.targetValue = targetValue;
    gameWork.currentValue = currentValue;
    gameWork.framesLeft = framesTotal;
    gameWork.framesTotal = framesTotal;
}

INCLUDE_ASM("asm/main/nonmatchings/tu089", func_00208B80);
