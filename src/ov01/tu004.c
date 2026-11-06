/*
 * OV01 original TU 4: 0x00a80660..0x00a807e8 (5 functions)
 */
#include "common.h"
#include "ov01/tu004.h"

/*
 * Two data object slots of 0x1e0 bytes at dataObjTable, the table that
 * dataObjCreate (func_00A89A08) allocates from through dataObjAlloc. The flags
 * word gates the two callbacks: update (dataObjUpdate, +0x4) runs unless flags
 * & 0x2, draw (dataObjDraw, +0x8) unless flags & 0x4. Each callback is called
 * with the slot it belongs to.
 */
typedef struct DataObjectSlot {
    int flags;
    void (*update)(struct DataObjectSlot *slot);
    void (*draw)(struct DataObjectSlot *slot);
    unsigned char unmodeled_0c[0x1d4];
} DataObjectSlot;

extern struct DataObjectSlot dataObjTable[2];
void ov01SlotRelease(void);

void unitRelease(void) {
    ov01SlotRelease();
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu004", dataObjResetAll);

/* Update sweep over the data object slots (called by the update sweep ov01UpdateAll). */
void dataObjUpdateAll(void) {
    struct DataObjectSlot *slot = dataObjTable;
    int i;

    for (i = 0; i < 2; i++) {
        if (slot->update != 0 && !(slot->flags & 2)) {
            slot->update(slot);
        }
        slot++;
    }
}

/* Draw sweep over the data object slots (called from the draw frame ov01DrawAll). */
void dataObjDrawAll(void) {
    struct DataObjectSlot *slot = dataObjTable;
    int i;

    for (i = 0; i < 2; i++) {
        if (slot->draw != 0 && !(slot->flags & 4)) {
            slot->draw(slot);
        }
        slot++;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu004", dataObjAlloc);
