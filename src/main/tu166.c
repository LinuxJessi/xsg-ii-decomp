#include "common.h"
#include "main/tu166.h"

/* Empty hook: nothing is done here. */
void func_00260840(void) {

}

/* Deletes the wait effect for the id (string "delete wait eft"): finds the matching entry of the eight-entry table
 * srsRecordTable and releases its record with sefReleaseEffectRecord. Called by sefReleaseWaitSlot (tu165). */
INCLUDE_ASM("asm/main/nonmatchings/tu166", sefDeleteWaitEffect);

/* Starts an effect from the 0x40-byte request at object: copies it into sefRequestBuffer, allocates a record with
 * func_00266BC8, passes value on to sefCreateEffectRecordDefault and sets flag bits in the word at +0x44 of the record in
 * sefEffectRecords. D_0069DC20 (0x3D or 0x3E) selects the path. */
INCLUDE_ASM("asm/main/nonmatchings/tu166", sefStartEffect);

void sefStartEffect(void *object, int value);

/* Starts the effect with value 0. */
void sefStartEffectDefault(void *object) {
    sefStartEffect(object, 0);
}

/* Starts the effect with the given value. */
void sefStartEffectWithValue(void *object, int value) {
    sefStartEffect(object, value);
}

/* Variant of sefStartEffect that copies the request the same way and returns
 * a pointer to the record it set up in sefEffectRecords, or NULL. */
INCLUDE_ASM("asm/main/nonmatchings/tu166", func_00260BD8);

void sefFreeEffectRecord(void);

/* Forwards to sefFreeEffectRecord. */
void func_00260D20(void) {
    sefFreeEffectRecord();
}
