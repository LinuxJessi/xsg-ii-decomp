/*
 * OV01 original TU 80: 0x00aaee00..0x00aaf718 (7 functions)
 */
#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu080", func_00AAEE00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu080", func_00AAEEE8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu080", func_00AAEFC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu080", func_00AAF0F8);

/* The block at +0x20 of the object these functions update. Only the fields
 * they access are modelled. status_window[0] holds the current status; the
 * functions shift the window by one entry per call. */
typedef struct MEfStatusBlock {
    unsigned char unmodeled_00[0x54];
    unsigned short step;
    unsigned char unmodeled_56[0x6A];
    unsigned short status_window[25];
} MEfStatusBlock;

typedef struct MEfStatusObject {
    unsigned char unmodeled_00[0x20];
    MEfStatusBlock block;
} MEfStatusObject;

short ov01LastValueGet(void);

void MEfSetCounterPhaseFlags(MEfStatusObject *object)
{
    MEfStatusBlock *block;
    short step;

    block = &object->block;
    if (ov01LastValueGet() == 0) {
        step = (short) block->step;
        block->status_window[0] = 0U;
        if (step < 10) {
            block->status_window[0] = 1U;
            if (step == 0) {
                block->status_window[0] = 0x11U;
            }
            if (step == 9) {
                block->status_window[0] |= 0x20;
            }
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu080", func_00AAF2D0);


void MEfAdvanceCounter(MEfStatusObject *object)
{
    MEfStatusBlock *block;
    int index;
    int next_step;

    block = &object->block;
    if (ov01LastValueGet() != 0) {
        return;
    }
    for (index = 24; index > 0; index--) {
        block->status_window[index] = block->status_window[index - 1];
    }
    next_step = block->step + 1;
    block->step = next_step;
    if ((short) next_step >= 0x23) {
        MSeObjRequestFree(object);
    }
}
