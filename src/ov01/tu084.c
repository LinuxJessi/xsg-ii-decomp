/*
 * OV01 original TU 84: 0x00ab1890..0x00ab1ce0 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/tu197.h"

/*
 * The flag-window block at object+0x20. Only the members these functions touch
 * are named; the gaps stay unmodeled. flags_window[0] holds the status word of
 * the current tick and [1..24] its history.
 */
typedef struct FlagWindowBlock FlagWindowBlock;

struct FlagWindowBlock {
    unsigned char unmodeled_00[0x54];
    s16 ticks;
    unsigned char unmodeled_56[0xC0 - 0x56];
    u16 flags_window[25];
};

typedef struct FlagWindowHost FlagWindowHost;

struct FlagWindowHost {
    unsigned char unmodeled_00[0x20];
    FlagWindowBlock block;
};

s16 ov01LastValueGet(void);

void MEfSetStepFlags30(FlagWindowHost *object) {
    FlagWindowBlock *block;
    s16 ticks;

    block = &object->block;
    if (ov01LastValueGet() == 0) {
        ticks = block->ticks;
        block->flags_window[0] = 0U;
        if (ticks < 0x1E) {
            block->flags_window[0] = 1U;
            if (ticks == 0) {
                block->flags_window[0] = 0x11U;
            }
            if (ticks == 0x1D) {
                block->flags_window[0] |= 0x20;
            }
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu084", MEfDrawFlagWindow);


void MEfAdvanceFlagWindow(FlagWindowHost *object) {
    FlagWindowBlock *block;
    int i;
    s16 ticks;

    block = &object->block;
    if (ov01LastValueGet() != 0) {
        return;
    }
    for (i = 24; i > 0; i--) {
        block->flags_window[i] = block->flags_window[i - 1];
    }
    ticks = block->ticks + 1;
    block->ticks = block->ticks + 1;
    if (ticks >= 0x37) {
        MSeObjRequestFree(object);
    }
}
