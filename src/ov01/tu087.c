/*
 * OV01 original TU 87: 0x00ab2940..0x00ab3210 (3 functions)
 */
#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu087", MEfStepSprayPoints);

INCLUDE_ASM("asm/nonmatchings/ov01/tu087", MEfDrawSpray);

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

void MEfTickLifetime121(LifetimeHost *object)
{
    LifetimeBlock *block;
    int ticks;

    block = &object->block;
    if (ov01LastValueGet() != 0) {
        return;
    }
    ticks = block->ticks + 1;
    block->ticks = ticks;
    if ((short) ticks >= 0x79) {
        MSeObjRequestFree(object);
    }
}
