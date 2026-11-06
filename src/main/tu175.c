#include "common.h"
#include "main/tu175.h"

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefOwnerInUse);

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_00264848);

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_002648C8);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefCountOwnerValue);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefClearOwnerValueFlags);

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_00264A80);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefOnlyOwnerInUse);

/*
 * One 0xA70-byte record of the table sefGetEffectRecords returns (the same table
 * as SefEffectRecord in sef_164.c). Only the two words sefReleaseOwnedRecords reads are
 * named: a nonzero in_use word, and the owner word compared with the owner
 * argument. tu168 reads +0x50 as its owner and +0x74 as a query key; the two
 * readings disagree and are left to the integrator.
 */
typedef struct OwnedSlot {
    unsigned char unmodeled_000[0x48];
    int in_use;                                        /* +0x48 */
    unsigned char unmodeled_04c[0x74 - 0x4c];
    int owner;                                         /* +0x74 */
    unsigned char unmodeled_078[0xa70 - 0x78];
} OwnedSlot;

void sefReleaseEffectRecord(int slot);
void *sefGetEffectRecords(void);
void sefReleaseByKey(int owner);
extern int sefEffectRecordCount;
extern int sefEffectRecordHead;

/*
 * Calls sefReleaseEffectRecord with the index of each record whose in_use word is set
 * and whose owner matches owner (every record when owner is negative), clears
 * sefEffectRecordHead when owner is negative, then calls sefReleaseByKey with owner.
 */
void sefReleaseOwnedRecords(int owner) {
    int slot;
    OwnedSlot *table;
    OwnedSlot *cursor;

    slot = 0;
    table = sefGetEffectRecords();
    if (owner < 0) {
        sefEffectRecordHead = 0;
    }
    if (sefEffectRecordCount > 0) {
        cursor = table;
        do {
            if (cursor->in_use != 0 && (owner < 0 || cursor->owner == owner)) {
                sefReleaseEffectRecord(slot);
            }
            slot += 1;
            cursor += 1;
        } while (slot < sefEffectRecordCount);
    }
    sefReleaseByKey(owner);
}

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_00264C28);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefApplyRecordMode);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefSetOwnerFlags);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefClearOwnerFlags);

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_00264EE8);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefUpdateGroupGate);

INCLUDE_ASM("asm/main/nonmatchings/tu175", sefSetGateForId);

INCLUDE_ASM("asm/main/nonmatchings/tu175", func_002652A8);
