#include "common.h"
#include "shared.h"

extern unsigned char uwamonoRecords[];

/*
 * One 0x870-byte record of uwamonoRecords, the uwamono record table (init_uwamono_sys.c
 * declares the same table as UwamonoRecord). kind 0 is a free record; the
 * scans below skip kind 4.
 */
typedef struct UwamonoRecord {
    unsigned char unmodeled_00[2];
    unsigned char kind;                 /* +0x02 */
    unsigned char unmodeled_03;
    int key;                            /* +0x04: compared by uwamonoFindRecord */
    unsigned char unmodeled_08[8];
    long long savedPositionXY;          /* +0x10: x, y of the saved position */
    long long savedPositionZW;          /* +0x18: z, w of the saved position */
    unsigned char unmodeled_20[0x80 - 0x20];
    long long homePositionXY;           /* +0x80: copy of the saved position, made by uwamonoRestoreRecord */
    long long homePositionZW;           /* +0x88 */
    unsigned char unmodeled_90[0x25C - 0x90];
    short field_25c;
    short countdown_25e;                /* +0x25e: set to 60 by uwamonoRestoreRecord */
    unsigned char unmodeled_260[0x570 - 0x260];
    int flags;                          /* +0x570: bit 0x8 cleared by uwamonoRestoreRecord */
    unsigned char unmodeled_574[0x62C - 0x574];
    signed char state;                  /* +0x62c: 0 when released, 2 counted by uwamonoCountState2 */
    unsigned char unmodeled_62d[0x63C - 0x62D];
    int handle_63c;                     /* released through sefReleaseRecordHandle */
    int handle_640;                     /* released through sefReleaseRecordHandle */
    unsigned char unmodeled_644[0x870 - 0x644];
} UwamonoRecord;

/* Counts the records from the first one up to the first free record (kind 0), at most 256. */
int uwamonoCountLeadingInUse(void)
{
    int count = 0;
    unsigned char *kindPtr;

    if (((UwamonoRecord *) uwamonoRecords)->kind != 0)
    {
        kindPtr = &((UwamonoRecord *) uwamonoRecords)->kind;
        for (;;)
        {
            count += 1;
            kindPtr += sizeof(UwamonoRecord);
            if (count < 0x100)
            {
                if (*kindPtr != 0)
                {
                    continue;
                }
            }
            break;
        }
    }
    return count;
}

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoReleaseRecord);

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoFindRecord);

/* One 0x40C0-byte record of D_002BD600; the leading id is -1 when the record is free. */
typedef struct SlotIdRecord {
    int id;
    unsigned char unmodeled_04[0x40C0 - 4];
} SlotIdRecord;

extern unsigned char D_002BD600[];

/* Counts the leading records of D_002BD600 whose id is not -1, at most 32. */
int slotIdRecordCountLeading(void)
{
    int count = 0;
    SlotIdRecord *record;

    if (((SlotIdRecord *) D_002BD600)->id != -1)
    {
        record = (SlotIdRecord *) D_002BD600;
        for (;;)
        {
            count += 1;
            record += 1;
            if (count < 0x20)
            {
                if (record->id != -1)
                {
                    continue;
                }
            }
            break;
        }
    }
    return count;
}

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoCountState2);

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoFindNearest);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FD7F8);

void sefReleaseRecordHandle(void *record);

/* Releases the two handles of every record that is not free and not kind 4, then clears them. */
void uwamonoReleaseHandles(void)
{
    UwamonoRecord *record = (UwamonoRecord *) uwamonoRecords;
    int index;

    for (index = 0; index < 256; index++)
    {
        if (record->kind != 0 && record->kind != 4)
        {
            int handle_63c = record->handle_63c;
            int handle_640;

            record->state = 0;
            if (handle_63c != 0)
            {
                sefReleaseRecordHandle(handle_63c);
            }
            handle_640 = record->handle_640;
            if (handle_640 != 0)
            {
                sefReleaseRecordHandle(handle_640);
            }
            record->handle_640 = 0;
            record->handle_63c = 0;
        }
        record++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FDA18);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FDE18);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FE298);

/* One 16-byte row of the table at uwamonoEntries: a pose with its angle and its key. */
typedef struct UwamonoEntry {
    float x;
    float y;
    float z;
    short angle;
    unsigned short key;
} UwamonoEntry;

extern unsigned short *uwamonoEntryCountPtr;
extern int uwamonoEntries;

/* Index of the row whose key is `key`, or -1. The row count is the short at uwamonoEntryCountPtr. */
int uwamonoFindEntry(int key)
{
    int result;
    int index;
    int found;

    found = -1;
    result = -1;
    if (uwamonoEntryCountPtr != 0)
    {
        for (index = 0; index < ((int) (*uwamonoEntryCountPtr)); index++)
        {
            if (((UwamonoEntry *) ((index * 0x10) + uwamonoEntries))->key == key)
            {
                found = index;
                break;
            }
        }
        result = found;
    }
    return result;
}

/* Angle codes are in 4096ths of a turn. */
extern float D_0069B030;

/* Writes the position of the row `key` to out (w = 1) and returns its angle; -1.0f when the table is empty. */
float uwamonoGetEntryPose(int key, float *out)
{
    float result = -1.0f;
    int entryOffset;
    UwamonoEntry *entry;
    float z;

    if (uwamonoEntryCountPtr != 0)
    {
        entryOffset = uwamonoFindEntry(key) * 16;
        if (out != 0)
        {
            entry = (UwamonoEntry *) (entryOffset + uwamonoEntries);
            out[0] = entry->x;
            out[1] = entry->y;
            z = entry->z;
            out[3] = 1.0f;
            out[2] = z;
        }
        result = (float) ((UwamonoEntry *) (entryOffset + uwamonoEntries))->angle * (1.0f / 4096.0f) * D_0069B030;
    }
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FE5B0);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FE830);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FEAC8);

void seVoiceStartDefault(int arg);

/* Restores the record at `index`: clears flags bit 0x8, arms the 60-tick countdown and copies the saved position home. */
void uwamonoRestoreRecord(int index)
{
    UwamonoRecord *record = &((UwamonoRecord *) uwamonoRecords)[index];
    long long savedPositionXY = record->savedPositionXY;
    long long savedPositionZW = record->savedPositionZW;

    record->flags &= ~8;
    record->countdown_25e = 0x3C;
    record->homePositionXY = savedPositionXY;
    record->homePositionZW = savedPositionZW;
    record->field_25c = 0;
    seVoiceStartDefault(0x02000002);
}

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FEF78);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FF3C0);

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoResetPoints);

INCLUDE_ASM("asm/main/nonmatchings/tu083", uwamonoAddPoint);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FFA60);

INCLUDE_ASM("asm/main/nonmatchings/tu083", func_001FFB70);
