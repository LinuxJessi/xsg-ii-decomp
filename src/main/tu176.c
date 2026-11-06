#include "common.h"
#include "shared.h"
#include "main/tu167.h"
#include "main/tu175.h"

/* Set by sefSetCurrentOwner after sefReleaseOwnedRecords(-1); this file has no other reader. */
extern int sefCurrentOwner;

/* Changes the current owner: releases every effect record, then stores owner in sefCurrentOwner. */
void sefSetCurrentOwner(int owner) {
    sefReleaseOwnedRecords(-1);
    sefCurrentOwner = owner;
}

/* Returns the current owner set by sefSetCurrentOwner. */
int sefGetCurrentOwner(void) {
    return sefCurrentOwner;
}

/* Clears the 48-slot handle table, calls srsReleaseOtherOwnerEffectSlots, then refills the
 * slots from the key records of srsRecordTable whose key is nonzero: eight
 * records first, then forty-eight more. Each refilled slot gets value 0. */
INCLUDE_ASM("asm/main/nonmatchings/tu176", sefResetHandleSlots);

void func_00265400(void) {

}

/* Registers handle with value in the first free slot, unless the handle is
 * already in the table. */
INCLUDE_ASM("asm/main/nonmatchings/tu176", sefRegisterHandle);

/* One slot of the 48-slot handle table sefHandleSlots: the value a handle was
 * registered with, and the handle. This file never reads the last four bytes. */
typedef struct HandleSlot {
    int value;
    int handle;
    u8 unmodeled_08[4];
} HandleSlot;

/* Predicate on a handle: nonzero for a type-1 handle whose id is in 300..449. */
int srsIsEffectImageKey();
extern HandleSlot sefHandleSlots[];

/* Clears every slot whose handle srsIsEffectImageKey accepts. */
void sefPruneHandleSlots(void) {
    HandleSlot *slot;
    int *handle_ptr;
    int handle;
    int i;

    slot = sefHandleSlots;
    handle_ptr = &slot->handle;
    for (i = 0; i < 48; i++) {
        handle = handle_ptr[0];
        if (handle != 0 && srsIsEffectImageKey(handle) != 0) {
            handle_ptr[-1] = 0;
            handle_ptr[0] = 0;
        }
        handle_ptr += 3;
    }
}


/* Calls sefStartEffectKeyDefault with the value and the handle of every registered slot. */
void sefDispatchHandleSlots(void) {
    HandleSlot *slot;
    int handle;
    int i;

    slot = sefHandleSlots;
    for (i = 0; i < 48; i++) {
        handle = slot->handle;
        if (handle != 0) {
            sefStartEffectKeyDefault(slot->value, handle);
        }
        slot++;
    }
}

/* Clears the sefEffectRecordList list, sets D_0069DB6C to 1 and calls sefResetHandleSlots.
 * Copies the in-use records that pass its flag and key tests into the list
 * (0x50-byte entries), registers their handles through sefRegisterHandle, and
 * passes the list and its count to func_00271398. */
INCLUDE_ASM("asm/main/nonmatchings/tu176", sefCollectEffectRecords);

/* Releases every record, dispatches the handle slots, then re-creates each
 * entry of the sefEffectRecordList list through sefSpawnEffectRecord and copies its fields back. */
INCLUDE_ASM("asm/main/nonmatchings/tu176", sefRestoreEffectRecords);

/* Calls sefGetEffectRecords, then spins for a count that follows sefEffectRecordCount. */
INCLUDE_ASM("asm/main/nonmatchings/tu176", func_002657D8);
