/*
 * OV01 original TU 82: 0x00aafa50..0x00ab1050 (15 functions)
 */
#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AAFA50);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AAFBF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfDrawQuadPacket);

/*
 * The lifetime block at object+0x20: a 16-bit tick count at +0x54. The tick
 * callback at object+0x14 advances it and ends the effect at its threshold.
 */
typedef struct LifetimeBlock {
    unsigned char unmodeled_00[0x54];
    unsigned short ticks;
} LifetimeBlock;

typedef struct LifetimeHost {
    unsigned char unmodeled_00[0x20];
    LifetimeBlock block;
} LifetimeHost;

short ov01LastValueGet(void);

void MEfTickLifetime28(LifetimeHost *object)
{
    LifetimeBlock *block;
    int ticks;

    block = &object->block;
    if (ov01LastValueGet() != 0) {
        return;
    }
    ticks = block->ticks + 1;
    block->ticks = ticks;
    if ((short) ticks >= 0x1C) {
        MSeObjRequestFree(object);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfCreatePathEffect31);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfMakeRotatedPath);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfFillHermiteSamples31);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AB04D0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AB0678);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AB0780);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfTickLifetime34);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfCreatePathEffect29);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfStepTrailPoints);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", MEfFillHermiteSamples29);

INCLUDE_ASM("asm/nonmatchings/ov01/tu082", func_00AB0E70);
