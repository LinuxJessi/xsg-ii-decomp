/*
 * OV01 original TU 85: 0x00ab1ce0..0x00ab2040 (4 functions)
 */
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu085", MEfCreateWaveEffect);

INCLUDE_ASM("asm/nonmatchings/ov01/tu085", MEfBlendSineSamples);

INCLUDE_ASM("asm/nonmatchings/ov01/tu085", MEfBuildSineSamples);

/*
 * The wave block at object+0x20. The code at +0x54 selects the status word
 * (flags, +0x270) and the mode (+0x272) that the tick callback writes.
 */
typedef struct WaveBlock {
    unsigned char unmodeled_00[0x54];
    short code;                       /* +0x54: selects the mode below */
    unsigned char unmodeled_56[0x270 - 0x56];
    unsigned short flags;             /* +0x270 */
    short mode;                       /* +0x272 */
} WaveBlock;

typedef struct WaveHost {
    unsigned char unmodeled_00[0x20];
    WaveBlock block;                  /* +0x20 */
} WaveHost;

short ov01LastValueGet(void);

void MEfSetWaveMode(WaveHost *object)
{
    WaveBlock *block;
    short code;

    block = &object->block;
    if (ov01LastValueGet() == 0) {
        code = block->code;
        block->flags = 0;
        if (code < 0x15) {
            block->flags = 1;
            block->mode = 1;
            if (code == 0) {
                block->mode = 0x11;
            }
            if (code == 0x14) {
                block->flags |= 0x20;
            }
        }
    }
}
