#include "common.h"
#include "shared.h"
#include "main/tu168.h"

void sefFreeEffectRecord();

/* One effect record of the table at sefEffectRecords: sefEffectRecordCount records of 0xA70 bytes. The same record type is
 * defined in sef_164.c; the two layouts must be reconciled by the integrator. */
typedef struct SefEffectRecord {
    unsigned char unmodeled_00[0x48];
    int inUse;                  /* 0x48: a record takes part only when nonzero */
    unsigned char unmodeled_4C[0x50 - 0x4C];
    int owner;                  /* 0x50: compared with the caller's owner id */
    unsigned char unmodeled_54[0x74 - 0x54];
    int queryKey;               /* 0x74: the key srsIsEffectImageKey is asked about */
    unsigned char unmodeled_78[0xA70 - 0x78];
} SefEffectRecord;

/* Releases one effect record. */
void sefReleaseRecordHandle(SefEffectRecord *record) {
    sefFreeEffectRecord(record);
}

INCLUDE_ASM("asm/main/nonmatchings/tu168", sefReleaseEntryRecords);

INCLUDE_ASM("asm/main/nonmatchings/tu168", func_002613A0);

extern SefEffectRecord *sefEffectRecords;
extern int sefEffectRecordCount;

/* Releases every in-use record whose owner is ownerId. */
void sefReleaseRecordsOfOwner(int ownerId)
{
    int i;

    if (ownerId != 0) {
        for (i = 0; i < sefEffectRecordCount; i++) {
            /* The record's byte offset from the table address. */
            SefEffectRecord *record = (SefEffectRecord *)(i * sizeof(SefEffectRecord) + (int)sefEffectRecords);

            if (record->inUse != 0 && record->owner == ownerId) {
                sefReleaseRecordHandle(record);
            }
        }
    }
}

int srsIsEffectImageKey(int queryKey);

/* Releases every in-use record whose query key passes srsIsEffectImageKey. */
void sefReleaseRecordsByQuery(void)
{
    int i;

    for (i = 0; i < sefEffectRecordCount; i++) {
        /* The record's byte offset from the table address. */
        SefEffectRecord *record = (SefEffectRecord *)(i * sizeof(SefEffectRecord) + (int)sefEffectRecords);

        if (record->inUse != 0 && srsIsEffectImageKey(record->queryKey) != 0) {
            sefReleaseRecordHandle(&sefEffectRecords[i]);
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu168", sefDestroyRecordEffects);

void sefDestroyRecordEffects();

void sefDestroyRecordEffectsClearFlag(int value)
{
    sefDestroyRecordEffects(value, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu168", sefResetEffects);

extern short D_0069DB80;

void func_00261778(void)
{
    D_0069DB80 = 1;
}

extern u8 sefRequestBuffer[];

u8 *sefGetRequestBuffer(void)
{
    return sefRequestBuffer;
}

/* One 0x2C0-byte entry of the table at sefPtAllocatorState. The getters return the
 * entry's data, which starts after a 16-byte prefix. */
typedef struct SefSlot {
    unsigned char unmodeled_00[0x10];
    unsigned char data[0x2C0 - 0x10];
} SefSlot;

extern SefSlot *sefPtAllocatorState;

unsigned char *func_00261798(int slotIndex)
{
    return sefPtAllocatorState[slotIndex].data;
}

/* Returns the data of slot slotIndex. func_00261798 has the same body and is left unnamed. */
unsigned char *sefGetSlotData(int slotIndex)
{
    return sefPtAllocatorState[slotIndex].data;
}

/* One 0x8B0-byte line-data record of the table at sefLineDataRecords (sefLineDataRecords in sef_164.c). */
typedef struct SefLineDataRecord {
    unsigned char unmodeled_00[0x8B0];
} SefLineDataRecord;

extern SefLineDataRecord *sefLineDataRecords;

/* Returns line-data record entryIndex, or null for a negative index. */
SefLineDataRecord *sefGetLineData(int entryIndex)
{
    if (entryIndex < 0) {
        return 0;
    }
    return &sefLineDataRecords[entryIndex];
}

/* Returns the base of the effect record table. */
SefEffectRecord *sefGetEffectRecords(void)
{
    return sefEffectRecords;
}

extern int sefCurrentRecord;

/* Returns the record most recently stored by sefDestroyRecordEffects (the current record). */
int sefGetCurrentRecord(void)
{
    return sefCurrentRecord;
}

/* The first index is only checked for a valid value; the slot is selected by the second. */
unsigned char *sefGetSlotDataChecked(int checkedIndex, int slotIndex)
{
    if (checkedIndex >= 0 && slotIndex >= 0) {
        return sefPtAllocatorState[slotIndex].data;
    }
    return 0;
}

int sefSetSlotDirection(void *destination, const Vector4 *direction, int slot);
extern u8 D_0068C220[];

/* The do-while keeps each call out of tail position, as the original keeps its frame. */
void sefSetSlotDirectionDefault(int first, int second)
{
    do {
        sefSetSlotDirection(first, second, D_0068C220);
    } while (0);
}

void sefSetSlotDirectionBetween();

void sefSetSlotDirectionBetweenDefault(int first, int second, int third)
{
    do {
        sefSetSlotDirectionBetween(first, second, third, D_0068C220);
    } while (0);
}
