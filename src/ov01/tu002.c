/*
 * OV01 original TU 2: 0x00a80368..0x00a804d8 (4 functions)
 */
#include "common.h"
#include "ov01/tu002.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu002", func_00A80368);

/*
 * One 0xA0-byte slot of the two-entry monsSlotTable (monsSlotTable). func_00A88AC0 (tu019, monsSlotNew) claims one
 * through func_00A80480 and stores the empty callback func_00A88B90 at +0x4 and its actor at +0x90, which this TU
 * does not model. Each callback runs for the slot unless its mask bit is set in flags: update (update sweep
 * monsSlotRunUpdate, monsSlotRunUpdate) unless 0x2, draw (draw sweep monsSlotRunDraw, monsSlotRunDraw) unless 0x4.
 */
typedef struct MonsSlot {
    int flags;
    int (*update)(struct MonsSlot *slot);
    int (*draw)(struct MonsSlot *slot);
    unsigned char unmodeled_00C[0x94];
} MonsSlot;

extern MonsSlot monsSlotTable[2];

/* Update sweep over the two slots (monsSlotRunUpdate). */
void monsSlotRunUpdate(void)
{
    int (*update)(MonsSlot *slot);
    MonsSlot *slot;
    int i;

    slot = monsSlotTable;
    for (i = 0; i < 2; i++) {
        update = slot->update;
        if (update != 0 && !(slot->flags & 0x2)) {
            update(slot);
        }
        slot++;
    }
}

/* Draw sweep over the two slots (monsSlotRunDraw). */
void monsSlotRunDraw(void)
{
    int (*draw)(MonsSlot *slot);
    MonsSlot *slot;
    int i;

    slot = monsSlotTable;
    for (i = 0; i < 2; i++) {
        draw = slot->draw;
        if (draw != 0 && !(slot->flags & 0x4)) {
            draw(slot);
        }
        slot++;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu002", func_00A80480);
