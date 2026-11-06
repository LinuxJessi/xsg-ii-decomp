/*
 * OV01 original TU 86: 0x00ab2040..0x00ab2940 (3 functions)
 */
#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu086", MEfDrawWaveEffect);

#include "shared.h"

/*
 * The history block at object+0x20 and the host record passed to MEfAdvanceHistoryTrail.
 * Only the evidenced regions are modeled: the u16 tick count at +0x54 of the
 * block and the 26-entry u16 history at +0x270 of the block. Bytes between them
 * are untouched here.
 */
typedef struct HistoryBlock HistoryBlock;

typedef struct HistoryHost HistoryHost;

struct HistoryBlock {
    unsigned char unmodeled_00[0x54];
    u16 ticks; /* +0x54 */
    unsigned char unmodeled_56[0x270 - 0x56];
    u16 history[26]; /* +0x270 */
};

struct HistoryHost {
    unsigned char unmodeled_00[0x20];
    HistoryBlock block; /* +0x20 */
};

s16 ov01LastValueGet(void);

void MEfAdvanceHistoryTrail(HistoryHost *host)
{
    HistoryBlock *block;
    s16 next_ticks;
    int i;


    block = &host->block;
    if (ov01LastValueGet() != 0)
    {
        return;
    }
    /* The history shift is written twice, as the original does. */
    for (i = 25; i > 0; i--)
    {
        host->block.history[i] = host->block.history[i - 1];
    }
    for (i = 25; i > 0; i--)
    {
        block->history[i] = block->history[i - 1];
    }
    next_ticks = block->ticks + 2;
    block->ticks = (u16) (block->ticks + 2);
    if (next_ticks >= 0x2F)
    {
        MSeObjRequestFree(host);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu086", MEfCreateSpray);
