/*
 * OV01 original TU 0: 0x00a80000..0x00a801e0 (9 functions)
 */
#include "common.h"
#include "ov01/tu000.h"

typedef struct CallbackSlot CallbackSlot;

typedef void (*SlotCallback)(CallbackSlot *slot);

/*
 * One 0x60-byte entry of the two-entry table at D_00ABB870. Bit 0x1 of flags
 * is the claim bit (ov01SlotClaim). update runs while flags & 2 is clear and
 * is called from the update sweep ov01CallbackSlotsRunUpdate; draw runs while flags & 4 is
 * clear. ov01CallbackSlotsRunDraw (draw sweep) has no caller inside this overlay.
 */
struct CallbackSlot {
    int flags;
    SlotCallback update;
    SlotCallback draw;
    unsigned char unmodeled_0C[0x54];
};

extern CallbackSlot D_00ABB870[];

void ov01SlotClear(CallbackSlot *slot) {
    slot->flags = 0;
    slot->update = 0;
    slot->draw = 0;
}

/* Claims a slot whose claim bit is clear: sets flags to 1 and returns 1. */
int ov01SlotClaim(int *flags) {
    if (!(*flags & 1)) {
        *flags = 1;
        return 1;
    }
    return 0;
}

/* Releases one slot: clears the claim bit and both callbacks. Same body as ov01SlotClear. */
void ov01SlotRelease(CallbackSlot *slot) {
    slot->flags = 0;
    slot->update = 0;
    slot->draw = 0;
}

/* Returns nonzero while the claim bit 0x1 of *flags is set. */
int ov01SlotInUse(int *flags) {
    if ((*flags & 1) != 0) {
        return 1;
    }
    return 0;
}

void ov01CallbackSlotsClear(void) {
    int i;

    for (i = 0; i < 2; i++) {
        ov01SlotClear(&D_00ABB870[i]);
    }
}

/* Update sweep over the two-entry table: runs each slot's update unless flags & 2. */
void ov01CallbackSlotsRunUpdate(void) {
    SlotCallback callback;
    int i;

    for (i = 0; i < 2; i++) {
        callback = D_00ABB870[i].update;
        if (callback != 0 && !(D_00ABB870[i].flags & 2)) {
            callback(&D_00ABB870[i]);
        }
    }
}

/* Draw sweep over the two-entry table: runs each slot's draw unless flags & 4. */
void ov01CallbackSlotsRunDraw(void) {
    SlotCallback callback;
    int i;

    for (i = 0; i < 2; i++) {
        callback = D_00ABB870[i].draw;
        if (callback != 0 && !(D_00ABB870[i].flags & 4)) {
            callback(&D_00ABB870[i]);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu000", func_00A80170);

void ov01CallbackSlotRelease(CallbackSlot *slot) {
    ov01SlotRelease(slot);
}
