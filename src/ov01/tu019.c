/*
 * OV01 original TU 19: 0x00a88a50..0x00a88c60 (9 functions)
 */
#include "common.h"
#include "ov01/tu003.h"
#include "ov01/tu019.h"

void *memset(void *, int, unsigned int);

/*
 * Two-entry table of MonsSlot records (proposed monsSlotTable) and its count
 * (proposed monsSlotCount). func_00A88BB0 fills a free entry and bumps the
 * count, func_00A88C08 clears one and drops the count.
 */
extern int monsActiveSlots[2];
extern int monsActiveCount;

/*
 * monsSlotReset (proposed monsSlotReset): empties the two-entry table and
 * sets its count to 0. Called by ov01SceneInit (unnamed).
 */
void monsSlotReset(void) {
    memset(monsActiveSlots, 0, 8);
    monsActiveCount = 0;
}

int func_00A88AC0(void);
int func_00A88BB0(int);

/*
 * monsSlotCreate (proposed monsSlotCreate): makes a new MonsSlot with
 * func_00A88AC0 and inserts it with func_00A88BB0. Returns the record, or 0
 * when func_00A88AC0 fails. Callers: ov01StreamOpen, ov01StepOpenHandles.
 */
int monsSlotCreate(void) {
    int slot;

    slot = func_00A88AC0();
    if (slot != 0) {
        func_00A88BB0(slot);
    }
    return slot;
}

/*
 * func_00A88AC0 (proposed monsSlotNew): allocates a MonsSlot with the empty
 * callback func_00A88B90, creates an actor with XenoActorAlloc and stores it
 * at +0x90, then hides it (bit 0x4 of +0xBC) and puts it at the origin.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu019", func_00A88AC0);

void XenoActorRelease(int);
int func_00A88C08(void *);

/* A MonsSlot record: the callback and link words are not modeled here. */
struct MonsSlot {
    unsigned char unmodeled_00[0x90];
    int actor; /* +0x90: the actor the record drives; typed int here because
                * this TU only passes it on. tu020 types it as a pointer. */
};

/*
 * monsSlotRelease (proposed monsSlotRelease): releases the actor (XenoActorRelease),
 * removes the record from the table (func_00A88C08) and calls monsSlotReleaseRecord
 * first. Called by func_00A96E38.
 */
void monsSlotRelease(struct MonsSlot *slot) {
    monsSlotReleaseRecord();
    XenoActorRelease(slot->actor);
    func_00A88C08(slot);
}

/*
 * func_00A88B60 (proposed monsSlotCopyParams): copies two words of the
 * second argument (+0x10 and +0x1C) into the record's actor at +0x5EC and
 * +0x5F0. Returns 1, or 0 when either argument is null. Caller: ov01StepBindAndShow
 * (tu038).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu019", func_00A88B60);

void func_00A88B90(void) {
}

/*
 * monsActiveSlotsGet (proposed monsSlotTableGet): hands back the two-entry table
 * and returns its length, 2. Callers: func_00A946C0, func_00A94EB0,
 * func_00A95810, func_00A96AA8, func_00A96E38, unitBodySetPosDir.
 */
int monsActiveSlotsGet(int **table) {
    *table = monsActiveSlots;
    return 2;
}

/*
 * func_00A88BB0 (proposed monsSlotInsert): stores the record in the first
 * free entry of the table and increments the count. Returns 1 when stored,
 * 0 when the table is full.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu019", func_00A88BB0);

/*
 * func_00A88C08 (proposed monsSlotRemove): clears the entry that holds the
 * record and decrements the count. Returns 1 when found, 0 otherwise.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu019", func_00A88C08);
