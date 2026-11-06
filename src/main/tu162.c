#include "common.h"

typedef unsigned int Quadword __attribute__((mode(TI)));

/* The draw list at svDrawList (tu157 reads the same record as its draw-list cursor): entries is the base of
 * the 16-byte slots, a word at +0x4 that is written through its own symbol D_006B4C34, and end_index is the
 * number of slots queued so far. The slots between begin_index and end_index are the pending draw steps. */
typedef struct DrawList {
    Quadword *entries;
    int unmodeled_4;
    int end_index;
} DrawList;

extern DrawList svDrawList;
extern int D_006B4C34;

void func_002594E8(int value)
{
    D_006B4C34 = value;
}

/* Base of the slot table. */
Quadword *drawListGetSlots(void)
{
    return svDrawList.entries;
}

/* The slot at end_index, the first one past the queued slots. */
Quadword *drawListGetNextSlot(void)
{
    return svDrawList.entries + svDrawList.end_index;
}

/* Advances end_index by one slot. */
void drawListCommitSlot(void)
{
    svDrawList.end_index = svDrawList.end_index + 1;
}

/* Advances end_index by increment slots. */
void drawListCommitSlots(int increment)
{
    svDrawList.end_index = svDrawList.end_index + increment;
}

/* Appends one slot passed by value. */
void drawListAppendSlot(Quadword value)
{
    Quadword *slot;

    /* the byte offset is formed first, then the data pointer is added */
    slot = (Quadword *) ((svDrawList.end_index * sizeof(Quadword)) + (int) svDrawList.entries);
    svDrawList.end_index = svDrawList.end_index + 1;
    *slot = value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu162", func_00259580);

/* Appends four consecutive slots copied from src. */
void drawListAppendFourSlots(const Quadword *src)
{
    Quadword *dst;

    /* the byte offset is formed first, then the data pointer is added */
    dst = (Quadword *) ((svDrawList.end_index * sizeof(Quadword)) + (int) svDrawList.entries);
    svDrawList.end_index = svDrawList.end_index + 4;
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

INCLUDE_ASM("asm/main/nonmatchings/tu162", drawListAppendSlotCopy);
