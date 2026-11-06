/*
 * OV01 original TU 3: 0x00a804d8..0x00a80660 (5 functions)
 */
#include "common.h"
#include "ov01/tu003.h"

/*
 * One 0x1F0-byte slot of the nine-entry table unitSideTable (unitSideTable). Bit 0x1 of flags is the claim bit
 * (func_00A80608, called by unitCreateSide); the claim stores unitUpdate at +0x4 and the draw callback at +0x8,
 * and unitRemove releases the slot through unitRelease (tu004). Update runs unless flags & 2 (update sweep
 * unitRunUpdate, unitRunUpdate), draw runs unless flags & 4 (draw sweep unitRunDraw, unitRunDraw).
 * The struct name is kept: tu010 and tu017 define a different struct UnitRecord, and tu033 a UnitSlot.
 */
typedef struct LargeCallbackSlot {
    int flags;
    void (*update)(struct LargeCallbackSlot *);
    void (*draw)(struct LargeCallbackSlot *);
    unsigned char unmodeled_0C[0x1F0 - 0x0C];
} LargeCallbackSlot;

void ov01SlotClear(void *slot);
void ov01SlotRelease(void);
extern LargeCallbackSlot unitSideTable[9];

/*
 * Releases one MonsSlot record (monsSlotReleaseRecord): the only caller is monsSlotRelease (tu019, monsSlotRelease),
 * which calls it before it releases the actor. Same body as the other release wrappers.
 */
void monsSlotReleaseRecord(void)
{
    ov01SlotRelease();
}

/* Clears all nine unit slots (unitClearAll), called by ov01ClearTables. */
void unitClearAll(void)
{
    int i;

    for (i = 0; i < 9; i++) {
        ov01SlotClear(&unitSideTable[i]);
    }
}

/* Update sweep over the nine slots (unitRunUpdate): runs each slot's update unless flags & 2. */
void unitRunUpdate(void)
{
    void (*update)(LargeCallbackSlot *);
    LargeCallbackSlot *slot;
    int i;

    slot = unitSideTable;
    for (i = 0; i < 9; i++) {
        update = slot->update;
        if (update != 0 && !(slot->flags & 2)) {
            update(slot);
        }
        slot++;
    }
}

/* Draw sweep over the nine slots (unitRunDraw): runs each slot's draw unless flags & 4. */
void unitRunDraw(void)
{
    void (*draw)(LargeCallbackSlot *);
    LargeCallbackSlot *slot;
    int i;

    slot = unitSideTable;
    for (i = 0; i < 9; i++) {
        draw = slot->draw;
        if (draw != 0 && !(slot->flags & 4)) {
            draw(slot);
        }
        slot++;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu003", func_00A80608);
