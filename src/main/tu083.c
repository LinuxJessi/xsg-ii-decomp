#include "common.h"
#include "shared.h"
#include "main/tu051.h"

/* uwamono records (tu051.c has the same kinds and states): kind 0 free, 1 talks, 2 searched. */
#define UWAMONO_RECORD_COUNT 256
#define UWAMONO_POINT_COUNT 16
#define UWAMONO_KIND_FREE 0
#define UWAMONO_KIND_TALK 1
#define UWAMONO_KIND_SEARCHED 2
/* record state: 0 out of reach, 1 selected, 2 in reach (tu051.h). */
#define UWAMONO_STATE_OUT_OF_REACH 0
#define UWAMONO_STATE_SELECTED 1
#define UWAMONO_STATE_IN_REACH 2
/* cellMask of a record that every cell reaches. */
#define UWAMONO_CELL_MASK_ALL 0xFF
/* effect spawned for the selection marker, stored at record +0x640. */
#define UWAMONO_EFFECT_SELECTION_MARKER 0x100C801E

void sefReleaseRecordHandle(void *record);
void uwamonoReleaseHandles(void);



/* The two poses of uwamonoSetPairMode, read through this union view (alias set 0). */
typedef union UwamonoPoses {
    Vector4 point[2];
    unsigned int bits[8];
} UwamonoPoses;

/* The 0x30-byte link block of a record (+0x540). */
struct UwamonoLink
{
    unsigned char unmodeled_00[6];
    short ownerIndex;                   /* +0x06: index of the record whose release releases this one */
    unsigned char unmodeled_08[0x30 - 0x08];
};

/*
 * One 0x870-byte record of uwamonoRecords as this file reads it (tu051.h's
 * UwamonoObject is the shared view; the table is the same). kind 0 is a free
 * record; the scans below skip kind 4.
 */
typedef struct UwamonoRecord {
    unsigned char unmodeled_00[2];
    unsigned char kind;                 /* +0x02 */
    unsigned char unmodeled_03;
    int key;                            /* +0x04: compared by uwamonoFindRecord */
    int type;                       /* +0x08: tested in 7..11 by func_001FFB70; -1 keeps the record out of uwamonoStartPairMove */
    unsigned char unmodeled_0c[4];
    long long savedPositionXY;          /* +0x10: x, y of the saved position */
    long long savedPositionZW;          /* +0x18: z, w of the saved position */
    unsigned char unmodeled_20[0x34 - 0x20];
    float scale34;                      /* +0x34: set to a literal by uwamonoStartPairMove */
    unsigned char unmodeled_38[0x44 - 0x38];
    float rotationY;                    /* +0x44: copied into poses by uwamonoStartPairMove */
    float rotationZ;                    /* +0x48 */
    unsigned char unmodeled_4c[0x50 - 0x4C];
    Vector4 extent50;                   /* +0x50 */
    unsigned char unmodeled_60[0x80 - 0x60];
    long long homePositionXY;           /* +0x80: copy of the saved position, made by uwamonoRestoreRecord */
    long long homePositionZW;           /* +0x88 */
    unsigned char unmodeled_90[0xA0 - 0x90];
    Vector4 turnEnds[2];                   /* +0xa0: written by uwamonoSetPairMode and uwamonoStartPairMove */
    unsigned char unmodeled_c0[0x216 - 0xC0];
    short poseElapsed;                  /* +0x216: cleared by uwamonoSetPairMode */
    short poseDuration;                 /* +0x218: copy of moveTime in uwamonoSetPairMode */
    unsigned char unmodeled_21a[0x25C - 0x21A];
    short moveFrame;
    short moveDuration;                /* +0x25e: set to 60 by uwamonoRestoreRecord */
    unsigned char unmodeled_260[0x330 - 0x260];
    struct XenoActor *actor;            /* +0x330 */
    int *motions;                    /* +0x334: the motion block; its first word is set to -1 on release (the scans read it as UwamonoMotionScan) */
    float form;                    /* +0x338: half of it bounds the distance test in func_001FFB70 */
    unsigned char unmodeled_33c[0x540 - 0x33C];
    struct UwamonoLink link_540;        /* +0x540 */
    int flags;                          /* +0x570: bit 0x8 cleared by uwamonoRestoreRecord */
    unsigned char unmodeled_574[0x580 - 0x574];
    float extentX;
    float extentY;
    unsigned char unmodeled_588[0x5A0 - 0x588];
    int effectId;                      /* +0x5a0: effect id for sefSpawnEffectById */
    union { int linkA; unsigned int partnerWordA; }; /* +0x5a4 (alias-0 view, style debt) */
    union { int linkB; unsigned int partnerWord; }; /* +0x5a8 (alias-0 view, style debt) */
    unsigned char unmodeled_5ac[0x5C0 - 0x5AC];
    signed char motionKind;             /* +0x5c0: 0 when idle, 1 starts uwamonoStartMove */
    signed char searchState;
    signed char pairMode;               /* +0x5c2: the mode uwamonoSetPairMode stores */
    unsigned char unmodeled_5c3;
    unsigned short moveTime;            /* +0x5c4 */
    unsigned char unmodeled_5c6[0x5D0 - 0x5C6];
    Vector4 statePosition[2];               /* +0x5d0: [0] is the anchor uwamonoFindTalkTarget reads */
    union {
        Vector4 point[2];
        float f[8];
    } rest;                             /* +0x5f0 */
    float reachRadius;
    unsigned char unmodeled_614[0x618 - 0x614];
    int linkIndex;                      /* +0x618: -1 when none */
    short tableIndex;
    unsigned char unmodeled_61e[0x620 - 0x61E];
    int triggerSound;                      /* +0x620: voice id for seVoiceStartDefault, -1 when none */
    int moveSound;                      /* +0x624: voice code of seVoiceStartDefault */
    unsigned char unmodeled_628[0x62C - 0x628];
    signed char state;                  /* +0x62c: 0 when released, 2 counted by uwamonoCountState2 */
    unsigned char unmodeled_62d[0x630 - 0x62D];
    int field_630;
    int cellMask;                       /* +0x634: bit set tested by uwamonoIsActorInCells; 0 and 0xFF are special */
    void *handles[3];                   /* +0x638, +0x63c, +0x640: released through sefReleaseRecordHandle */
    int releaseNextIndex;                   /* +0x644: released after this one, -1 for none */
    unsigned char unmodeled_648[0x870 - 0x648];
} UwamonoRecord;

/* Counts the records from the first one up to the first free record (kind 0), at most 256. */
int uwamonoCountLeadingInUse(void)
{
    int count = 0;
    unsigned char *kindPtr;

    if (((UwamonoRecord *) uwamonoRecords)->kind != UWAMONO_KIND_FREE)
    {
        kindPtr = &((UwamonoRecord *) uwamonoRecords)->kind;
        for (;;)
        {
            count += 1;
            kindPtr += sizeof(UwamonoRecord);
            if (count < UWAMONO_RECORD_COUNT)
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




short uwamonoFindSelected(void);
void uwamonoResetRecords(int index, int flag);
void XenoActorRelease(struct XenoActor *actor);
extern int D_002B7CA8[];
extern int D_002B7C88[];

void uwamonoReleaseRecord(int index)
{
    UwamonoRecord *record;
    struct UwamonoLink *link;
    int recordIndex;
    int ownedIndex;

    if (index == -1)
    {
        return;
    }
    for (recordIndex = 0; recordIndex < UWAMONO_RECORD_COUNT; recordIndex++)
    {
        record = &((UwamonoRecord *) uwamonoRecords)[recordIndex];
        if (record->kind != UWAMONO_KIND_FREE && (record->link_540.ownerIndex == index || recordIndex == index))
        {
        for (ownedIndex = 0; ownedIndex < UWAMONO_RECORD_COUNT; ownedIndex++)
        {
            link = &((UwamonoRecord *) uwamonoRecords)[ownedIndex].link_540;
            if (link->ownerIndex == recordIndex)
            {
                uwamonoReleaseRecord(ownedIndex);
            }
        }
        if (record->releaseNextIndex != -1)
        {
            uwamonoReleaseRecord(record->releaseNextIndex);
        }
        if (uwamonoFindSelected() == index)
        {
            uwamonoReleaseHandles();
        }
        switch (record->kind)
        {
        case UWAMONO_KIND_TALK:
        case 4:
        case 5:
            uwamonoRecords[recordIndex].actor->drawFlags |= 4;
            XenoActorRelease(uwamonoRecords[recordIndex].actor);
            record->actor = 0;
            break;
        case UWAMONO_KIND_SEARCHED:
            if (record->key != -1)
            {
                D_002B7CA8[record->key >> 5] |= 1 << (record->key & 0x1F);
            }
            break;
        case 3:
            D_002B7C88[record->key >> 5] |= 1 << (record->key & 0x1F);
            break;
        case 6:
            break;
        }
        if (record->handles[0] != 0)
        {
            sefReleaseRecordHandle(record->handles[0]);
        }
        if (record->handles[1] != 0)
        {
            sefReleaseRecordHandle(record->handles[1]);
        }
        if (record->handles[2] != 0)
        {
            sefReleaseRecordHandle(record->handles[2]);
        }
        record->kind = 0;
        record->handles[1] = 0;
        record->handles[2] = 0;
        record->handles[0] = 0;
        if (((UwamonoRecord *) uwamonoRecords)[recordIndex].motions != 0)
        {
            *((UwamonoRecord *) uwamonoRecords)[recordIndex].motions = -1;
        }
        uwamonoResetRecords(recordIndex, 1);
        }
    }
}

/* Index of the first record whose kind and key match, or 256 when none does. */
int uwamonoFindRecord(int kind, int key)
{
    int index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++)
    {
        UwamonoObject *record = &uwamonoRecords[index];

        if (record->kind == kind && record->key == key)
        {
            break;
        }
    }
    return index;
}

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

/* Counts the records whose kind is not 0 and whose state is 2. */
int uwamonoCountState2(void)
{
    short count = 0;
    short index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++)
    {
        UwamonoObject *record = &uwamonoRecords[index];

        if (record->kind != UWAMONO_KIND_FREE && uwamonoRecords[index].state == 2)
        {
            count++;
        }
    }
    return count;
}

float Get_Distance(const Vector4 *from, const Vector4 *to);

/* Index of the in-use record with a nonzero state nearest to uwamonoCurrentIndex, or -1; the search starts at 1000.0f. */
int uwamonoFindNearest(void)
{
    int nearest = -1;
    float nearestDistance = 1000.0f;
    short index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++)
    {
        UwamonoObject *record = &uwamonoRecords[index];

        if (record->kind != UWAMONO_KIND_FREE && uwamonoRecords[index].state != 0)
        {
            float distance = Get_Distance(&uwamonoRecords[uwamonoCurrentIndex].position, &uwamonoRecords[index].position);

            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = index;
            }
        }
    }
    return nearest;
}

float Get_Distance3D(const void *first, const void *second);

/* The gameWork words this file reads (tu069 MapColorBlock and tu077 CameraWork are other views). */
typedef struct UwamonoGameControl {
    unsigned char unmodeled_000[6];
    unsigned short field_006;
    int control;                        /* +0x08 */
    unsigned char unmodeled_00c[0x1C - 0x0C];
    int field_1c;                       /* +0x1C */
    unsigned char unmodeled_020[0x2D8 - 0x20];
    float searchRadius;                 /* +0x2d8 */
    unsigned char unmodeled_2dc[0x2E0 - 0x2DC];
    float searchHeight;                 /* +0x2e0 */
    unsigned char unmodeled_2e4[0x1520 - 0x2E4];
    char flags[8];                      /* +0x1520 */
} UwamonoGameControl;

extern UwamonoGameControl gameWork;

short uwamonoFindSelected(void)
{
    short result = -1;
    short index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++)
    {
        UwamonoObject *record = &uwamonoRecords[index];

        if (record->kind != UWAMONO_KIND_FREE && record->kind != 4)
        {
            float distance;
            double height;
            float heightDistance;

            distance = Get_Distance3D(&uwamonoRecords[uwamonoCurrentIndex].actor->position, &uwamonoRecords[index].position);
            height = uwamonoRecords[uwamonoCurrentIndex].actor->position.y - uwamonoRecords[index].position.y;
            if (height < 0)
            {
                height = 0.0 - height;
            }
            heightDistance = height;
            if (!(gameWork.searchRadius < distance) && !(gameWork.searchHeight < heightDistance)
                && uwamonoRecords[index].state == 1)
            {
                result = index;
                break;
            }
        }
    }
    return result;
}

/* Releases the two handles of every record that is not free and not kind 4, then clears them. */
void uwamonoReleaseHandles(void)
{
    UwamonoObject *record = uwamonoRecords;
    int index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++)
    {
        if (record->kind != UWAMONO_KIND_FREE && record->kind != 4)
        {
            void *handle_63c = record->handles[1];
            void *handle_640;

            record->state = UWAMONO_STATE_OUT_OF_REACH;
            if (handle_63c != 0)
            {
                sefReleaseRecordHandle(handle_63c);
            }
            handle_640 = record->handles[2];
            if (handle_640 != 0)
            {
                sefReleaseRecordHandle(handle_640);
            }
            record->handles[2] = 0;
            record->handles[1] = 0;
        }
        record++;
    }
}

void func_00202E60(Vector4 *first, Vector4 *second, int mode);
int uwamonoCanSeeRecord(UwamonoRecord *record);
int sefSpawnEffectRecordById(unsigned int id, Vector4 *pos, int flags);
void sefDestroyRecordEffectsClearFlag(int handle);
void uwamonoCrossPoint(Vector4 *from, Vector4 *to, float radius, Vector4 *out);
void uwamonoMoveSelection(int step);
void seVoiceStartDefault(int arg);
extern int D_0069B964;
extern signed char uwamonoDrawDisabled;
extern short uwamonoLadderState;
extern int mainObject;

void uwamonoUpdateInReach(struct XenoActor *actor, int index)
{
    UwamonoRecord *record;
    UwamonoObject *object;
    int slot;
    Vector4 saved;
    Vector4 hit;
    Vector4 *actorPos;
    Vector4 *recordPos;
    int result;
    int count;
    double height;
    int state;
    float distance;
    float heightDistance;

    slot = index;
    object = &uwamonoRecords[slot];
    record = &((UwamonoRecord *) uwamonoRecords)[slot];
    if (record->kind == UWAMONO_KIND_FREE || record->kind == 4 || record->kind == 6)
    {
        return;
    }
    if (record->flags & 0x2000000)
    {
        return;
    }
    if (gameWork.field_006 != 0)
    {
        return;
    }
    if (D_0069B964 == 0)
    {
        return;
    }
    if (mainObject == 0)
    {
        return;
    }
    if (gameWork.control & 0x20)
    {
        if (gameWork.field_1c != -1)
        {
            return;
        }
    }
    actorPos = &actor->position;
    recordPos = (Vector4 *) &record->savedPositionXY;
    distance = Get_Distance3D(actorPos, recordPos);
    height = actor->position.y - object->position.y;
    if (height < 0)
    {
        height = -height;
    }
    heightDistance = height;
    *(long long *) &saved.x = *(long long *) recordPos;
    *(long long *) &saved.z = record->savedPositionZW;

    if ((unsigned int) (actor->kind - 0x10000) < 0x4000)
    {
        if (record->flags & 0x100000)
        {
            return;
        }
    }
    else
    {
        if (!(record->flags & 0x100000))
        {
            return;
        }
    }
    if (!(record->flags & 0x20000))
    {
        return;
    }
    result = 1;
    if (!(record->flags & 0x200000))
    {
        result = (signed char) uwamonoCanSeeRecord(&((UwamonoRecord *) uwamonoRecords)[index]);
    }
    if (result == 1)
    {
        func_00202E60(actorPos, &saved, 7);
    }
    else if (uwamonoDrawDisabled == 0)
    {
        uwamonoCrossPoint(actorPos, &saved, actor->radius, &hit);
        func_00202E60(actorPos, &hit, 1);
    }

    if (distance <= gameWork.searchRadius && heightDistance <= gameWork.searchHeight
        && result == 1 && uwamonoLadderState == 0 && uwamonoIsActorInCells(index) == result)
    {
        if (record->state == UWAMONO_STATE_OUT_OF_REACH)
        {
            record->state = UWAMONO_STATE_IN_REACH;
            record->field_630 = 0;
            if (record->handles[1] == 0)
            {
                record->handles[1] = (void *) sefSpawnEffectRecordById(0x100C801D, &saved, 0);
            }
            if (record->handles[2] == 0)
            {
                record->handles[2] = (void *) sefSpawnEffectRecordById(0x100C801E, &saved, 0);
            }
            seVoiceStartDefault(0x2000003);
        }
        count = uwamonoCountState2();
        if (count != 1)
        {
            return;
        }
        if (uwamonoFindSelected() != -1)
        {
            return;
        }
        record->state = count;
        if (record->handles[1] == 0)
        {
            record->handles[1] = (void *) sefSpawnEffectRecordById(0x100C801D, &saved, 0);
        }
        if (record->handles[2] == 0)
        {
            record->handles[2] = (void *) sefSpawnEffectRecordById(0x100C801E, &saved, 0);
        }
        sefDestroyRecordEffectsClearFlag((int) record->handles[1]);
        sefDestroyRecordEffectsClearFlag((int) record->handles[2]);
        return;
    }

    state = record->state;
    if (state == 1)
    {
        record->state = UWAMONO_STATE_OUT_OF_REACH;
        sefReleaseRecordHandle(record->handles[1]);
        sefReleaseRecordHandle(record->handles[2]);
        record->handles[2] = 0;
        record->handles[1] = 0;
        uwamonoMoveSelection(1);
    }
    else if (state == 2)
    {
        record->state = UWAMONO_STATE_OUT_OF_REACH;
        sefReleaseRecordHandle(record->handles[1]);
        sefReleaseRecordHandle(record->handles[2]);
        record->handles[2] = 0;
        record->handles[1] = 0;
    }
    else
    {
        record->state = UWAMONO_STATE_OUT_OF_REACH;
    }
}

/* An effect record as the selection marker code reads it: its scale follows the player's form. */
typedef struct UwamonoMarker {
    unsigned char unmodeled_00[0x20];
    Vector4 scale;                      /* +0x20 */
} UwamonoMarker;

/* The four words at +0x634 of a record (cellMask and the three effect handles), addressed as one block. */
typedef struct UwamonoHandleBlock {
    int cellMask;                       /* record +0x634 */
    void *handle_638;
    void *handle_63c;
    UwamonoMarker *selected;            /* record +0x640: the marker shown on the selected record */
} UwamonoHandleBlock;

/*
 * Field `member` of record `index`, reached as the original does: the record offset and the field offset are summed
 * as one integer and the table address is added last, so every use computes an address of its own (the copies of the
 * record pointer in uwamonoMoveSelection come from that). `uwamonoRecords[index].member` adds the table first.
 */
#define UWAMONO_FIELD(index, member)                                                                   \
    (*({                                                                                               \
        int byteOffset = (index) * sizeof(UwamonoObject) + (int) &((UwamonoObject *) 0)->member;       \
        (__typeof__(((UwamonoObject *) 0)->member) *) ((char *) uwamonoRecords + byteOffset);          \
    }))

/*
 * Moves the selection by `step` records over the records in reach: forward for a positive step, backward for a
 * negative one, and to the nearest record for 0. Stepping puts the old selection back to "in reach", releases its
 * marker, spawns a new marker above it (scaled by the player's form) and selects the record found.
 */
void uwamonoMoveSelection(int step)
{
    int count = uwamonoCountState2();
    short current = uwamonoFindSelected();
    int moved;
    int probe;
    int index;
    int probeSum;
    UwamonoMarker *marker;
    Vector4 position;

    if (current == -1)
    {
        if (count == 0)
        {
            return;
        }
        for (current = 0; current < UWAMONO_RECORD_COUNT; current++)
        {
            if (uwamonoRecords[current].kind != UWAMONO_KIND_FREE
                && uwamonoRecords[current].kind != 4
                && uwamonoRecords[current].kind != 6
                && uwamonoRecords[current].state == UWAMONO_STATE_IN_REACH)
            {
                break;
            }
        }
    }
    if (step == 0)
    {
        int nearest = uwamonoFindNearest();

        if (nearest == current || nearest == -1)
        {
            return;
        }
        uwamonoRecords[current].state = UWAMONO_STATE_IN_REACH;
        current = nearest;
        uwamonoRecords[current].state = UWAMONO_STATE_SELECTED;
    }
    else if (step >= 0)
    {
        for (moved = 0; moved < step; moved++)
        {
            probe = 1;
            do
            {
                index = (current + probe) - ((current + probe) / 256) * 256;
                probe++;
                if (uwamonoRecords[index].kind != UWAMONO_KIND_FREE && uwamonoRecords[index].kind != 4
                    && uwamonoRecords[index].state != UWAMONO_STATE_OUT_OF_REACH)
                {
                    UwamonoHandleBlock *handles;

                    uwamonoRecords[current].state = UWAMONO_STATE_IN_REACH;
                    handles = (UwamonoHandleBlock *) &UWAMONO_FIELD(current, cellMask);
                    sefReleaseRecordHandle(handles->selected);
                    position.x = UWAMONO_FIELD(current, position.x) + UWAMONO_FIELD(current, focusOffset.x);
                    position.y = UWAMONO_FIELD(current, position.y) + UWAMONO_FIELD(current, focusOffset.y)
                                 + UWAMONO_FIELD(current, extent.y) * 0.5f;
                    position.z = UWAMONO_FIELD(current, position.z) + UWAMONO_FIELD(current, focusOffset.z);
                    marker = (UwamonoMarker *) sefSpawnEffectRecordById(UWAMONO_EFFECT_SELECTION_MARKER, &position, 0);
                    handles->selected = marker;
                    current = index;
                    marker->scale.x = marker->scale.y = marker->scale.z = uwamonoRecords[uwamonoCurrentIndex].form;
                    uwamonoRecords[current].state = UWAMONO_STATE_SELECTED;
                    break;
                }
            } while (probe < 257);
        }
    }
    else
    {
        for (moved = 0; moved < -step; moved++)
        {
            probe = 1;
            do
            {
                probeSum = current - probe + 256;
                index = probeSum - (probeSum / 256) * 256;
                probe++;
                if (uwamonoRecords[index].kind != UWAMONO_KIND_FREE && uwamonoRecords[index].kind != 4
                    && uwamonoRecords[index].state != UWAMONO_STATE_OUT_OF_REACH)
                {
                    UwamonoHandleBlock *handles;

                    uwamonoRecords[current].state = UWAMONO_STATE_IN_REACH;
                    handles = (UwamonoHandleBlock *) &UWAMONO_FIELD(current, cellMask);
                    sefReleaseRecordHandle(handles->selected);
                    position.x = UWAMONO_FIELD(current, position.x) + UWAMONO_FIELD(current, focusOffset.x);
                    position.y = UWAMONO_FIELD(current, position.y) + UWAMONO_FIELD(current, focusOffset.y)
                                 + UWAMONO_FIELD(current, extent.y) * 0.5f;
                    position.z = UWAMONO_FIELD(current, position.z) + UWAMONO_FIELD(current, focusOffset.z);
                    marker = (UwamonoMarker *) sefSpawnEffectRecordById(UWAMONO_EFFECT_SELECTION_MARKER, &position, 0);
                    handles->selected = marker;
                    current = index;
                    marker->scale.x = marker->scale.y = marker->scale.z = uwamonoRecords[uwamonoCurrentIndex].form;
                    uwamonoRecords[current].state = UWAMONO_STATE_SELECTED;
                    break;
                }
            } while (probe < 257);
        }
    }
    seVoiceStartDefault(0x02000004);
}

int func_002A6D88(float value);
int func_002A7510(int a, int b);
int func_002A7080(int a, int b);
float func_002A76E8(int value);

typedef struct UwamonoBox {
    Vector4 min;
    Vector4 max;
} UwamonoBox;

typedef struct UwamonoCellBits {
    unsigned char bit[9];
} UwamonoCellBits;

extern UwamonoCellBits uwamonoCellBitTable;

int uwamonoIsActorInCells(int index)
{
    UwamonoBox box;
    UwamonoCellBits cellBits;
    UwamonoObject *target = &uwamonoRecords[index];
    struct XenoActor *actor = uwamonoRecords[uwamonoCurrentIndex].actor;
    float left;
    float right;
    float back;
    float front;
    int cellMask;
    int xCell;
    int zCell;
    int cellBit;

    cellBits = uwamonoCellBitTable;
    cellMask = ((UwamonoRecord *) uwamonoRecords)[index].cellMask;
    if (cellMask == UWAMONO_CELL_MASK_ALL)
    {
        return 1;
    }
    if (cellMask == 0)
    {
        return 0;
    }

    left = target->position.x - target->extent.x;
    right = target->position.x + target->extent.x;
    back = target->position.z - target->extent.z;
    front = target->position.z + target->extent.z;

    box.max.y = target->position.y;
    box.min.y = target->position.y;
    box.min.x = left - 0.5f;
    box.max.x = right + 0.5f;
    box.max.z = back;
    box.min.z = back;
    func_00202E60(&box.min, &box.max, 0);

    box.max.z = front;
    box.min.z = front;
    func_00202E60(&box.min, &box.max, 0);

    box.max.x = left;
    box.min.x = left;
    box.min.z = back - 0.5f;
    box.max.z = front + 0.5f;
    func_00202E60(&box.min, &box.max, 0);

    box.max.x = right;
    box.min.x = right;
    func_00202E60(&box.min, &box.max, 0);

    if (actor->position.x < left)
    {
        xCell = 0;
    }
    else if (right < actor->position.x)
    {
        xCell = 2;
    }
    else
    {
        xCell = 1;
    }

    if (actor->position.z < back)
    {
        zCell = 0;
    }
    else if (front < actor->position.z)
    {
        zCell = 2;
    }
    else
    {
        zCell = 1;
    }

    cellBit = 1 << cellBits.bit[zCell * 3 + xCell];

    return (target->cellMask & cellBit) != 0;
}

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

void uwamonoStartMove(int index, Vector4 *point, short time, int mode, int aux, int flag);

void uwamonoSetPairMode(int index, int mode)
{
    UwamonoRecord *pair[2];
    int recordIds[2];
    int ownRow;
    int partnerRow;
    int side;
    int ownZOff;
    int partnerZOff;
    int ownRowOff;
    int partnerRowOff;
    int partnerPoseOff;

    pair[0] = (UwamonoRecord *) &uwamonoRecords[index];
    if (pair[0]->linkIndex != -1)
    {
        pair[1] = (UwamonoRecord *) &uwamonoRecords[uwamonoRecords[index].linkIndex];
    }
    if (pair[0]->motionKind != 0)
    {
        recordIds[0] = index;
        recordIds[1] = uwamonoRecords[index].linkIndex;
        if (pair[0]->moveFrame == -1 && (pair[0]->linkIndex == -1 || pair[1]->moveFrame == -1))
        {
            if (mode == 2)
            {
                mode = (pair[0]->pairMode == 0);
            }
            if (pair[0]->pairMode != mode)
            {
                if (mode == 0)
                {
                    ownRow = 0;
                    partnerRow = 1;
                }
                else
                {
                    ownRow = 1;
                    partnerRow = 0;
                }
                ownRowOff = ownRow * 16;
                partnerRowOff = partnerRow * 16;
                partnerPoseOff = partnerRowOff + (((unsigned int) partnerRowOff & 0xFF) >> 8);
                side = 0;
                ownZOff = ownRowOff + 0x5f8;
                partnerZOff = partnerPoseOff + 0x5f8;
                for (; side < 2 && (side != 1 || pair[0]->linkIndex != -1); side++)
                {
                    pair[side]->moveFrame = 0;
                    pair[side]->moveDuration = pair[side]->moveTime;
                    if (side == 0 && pair[0]->moveDuration != 0)
                    {
                        seVoiceStartDefault(pair[0]->moveSound);
                    }
                    if (pair[side]->motionKind == 1)
                    {
                        uwamonoStartMove(recordIds[side], &pair[side]->statePosition[partnerRow], (short) pair[side]->moveTime, 0, -1, 0);
                    }
                    else
                    {
                        pair[side]->poseElapsed = 0;
                        pair[side]->poseDuration = pair[side]->moveTime;
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[0].x = pair[side]->rest.point[ownRow].x;
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[0].y = *(float *)(ownRowOff + (int) pair[side] + 0x5f4);
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[0].z = *(float *)((char *) pair[side] + ownZOff);
                        (void) ownRowOff;
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[0].w = 1.0f;
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[1].x = *(float *)(partnerPoseOff + (int) pair[side] + 0x5f0);
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[1].y = *(float *)(partnerPoseOff + (int) pair[side] + 0x5f4);
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[1].z = *(float *)((char *) pair[side] + partnerZOff);
                        ((UwamonoPoses *) pair[side]->turnEnds)->point[1].w = 1.0f;
                    }
                    pair[side]->pairMode = mode;
                }
            }
        }
    }
}

extern float D_0069B034;
extern float D_0069B038;
void func_001F0750(int index, Vector4 *point, int time);
void uwamonoRestoreRecord(int index);

/* Sets the pose and the pair mode of record `index`, then hands its partner the saved position and a move. */
void uwamonoStartPairMove(int index, int mode, int duration)
{
    UwamonoRecord *record = &((UwamonoRecord *) uwamonoRecords)[index];
    Vector4 ones;

    if (record->kind != UWAMONO_KIND_SEARCHED)
    {
        return;
    }
    if (record->type == -1)
    {
        return;
    }
    if (record->moveFrame != -1)
    {
        return;
    }
    if ((record->flags & 0x02000000) != 0)
    {
        return;
    }
    if (mode == 5)
    {
        mode = (record->pairMode == 3) ? 4 : 3;
    }
    else if (mode == 3)
    {
        mode = 3;
    }
    if (mode != 3)
    {
        return;
    }
    if (mode != 3)
    {
        return;
    }
    if (duration >= 2)
    {
        seVoiceStartDefault(0x0200000F);
    }
    record->moveDuration = duration;
    record->moveFrame = 0;
    record->turnEnds[0].x = 0.0f;
    record->turnEnds[0].y = record->rotationY;
    record->turnEnds[0].z = record->rotationZ;
    record->turnEnds[0].w = 1.0f;
    record->turnEnds[1].x = D_0069B034;
    record->turnEnds[1].y = record->rotationY;
    record->turnEnds[1].z = record->rotationZ;
    record->pairMode = mode;
    record->turnEnds[1].w = 1.0f;
    if (record->linkB == -1)
    {
        return;
    }
    {
        int offset = record->linkB * sizeof(UwamonoRecord) + 0x10;
        DoublewordVector4 *destination = (DoublewordVector4 *) ((unsigned char *) uwamonoRecords + offset);
        destination->words[0] = ((DoublewordVector4 *) &record->savedPositionXY)->words[0];
        destination->words[1] = ((DoublewordVector4 *) &record->savedPositionXY)->words[1];
    }
    {
        int offset = record->linkB * sizeof(UwamonoRecord) + 0x14;
        float *y = (float *) ((unsigned char *) uwamonoRecords + offset);
        *y += 0.5f;
    }
    {
        int xOffset = 80;
        int yOffset = 84;
        int zOffset = 88;
        (*(float *) ((unsigned char *) uwamonoRecords + record->linkB * sizeof(UwamonoRecord) + xOffset)) = (*(float *) ((unsigned char *) uwamonoRecords + record->linkB * sizeof(UwamonoRecord) + yOffset)) = (*(float *) ((unsigned char *) uwamonoRecords + record->linkB * sizeof(UwamonoRecord) + zOffset)) = 0.0f;
    }
    ((UwamonoRecord *) uwamonoRecords)[record->linkB].extent50.w = 1.0f;
    ones.x = ones.y = ones.z = 1.0f;
    ones.w = 1.0f;
    func_001F0750(record->linkB, &ones, 20);
    ((UwamonoRecord *) uwamonoRecords)[record->linkB].scale34 = D_0069B038;
    {
        uwamonoRecords[record->linkB].flags |= 2;
    }
    {
        uwamonoRecords[record->linkB].flags &= 0xFDFFFFFF;
    }
    uwamonoRestoreRecord(record->linkB);
    record->linkB = -1;
}

/* The motion block of a record (+0x334), with the two fields the point scan reads. */
typedef struct UwamonoMotionScan {
    unsigned char unmodeled_00[0x0C];
    signed char actionState;            /* +0x0C: 6 and 9 are skipped by the point scan */
    unsigned char unmodeled_0d[0x60 - 0x0D];
    short hold;                         /* +0x60: nonzero skips the record */
} UwamonoMotionScan;

/* The subject of the search (arg0): a position at +0x10, a heading at +0x54, two more fields. */
typedef struct UwamonoSubject {
    unsigned char unmodeled_00[0x10];
    Vector4 position;                   /* +0x10 */
    unsigned char unmodeled_20[0x54 - 0x20];
    float heading;                      /* +0x54 */
    unsigned char unmodeled_58[0xB0 - 0x58];
    float subjectRadius;                     /* +0xB0 */
    unsigned char unmodeled_b4[0x798 - 0xB4];
    int actorKind;                      /* +0x798 */
} UwamonoSubject;



extern UwamonoRecord D_00340390[];
extern int uwamonoCurrentIndex;
void Get_MiddlePoint(void *a, void *b, int c, int d, void *out);
int unduCheckAngle(float a, float b, float c);
int unduCheckInsideFan(int a, void *b, float e, float f, void *c, int d, float g, float h);

/* The nearest target of the subject (arg0) within range: a record index, or -1. */
int uwamonoFindTalkTarget(UwamonoSubject *subject, float range)
{
    int bestIndex = -1;
    float bestDist = 9999.9f;
    int index;
    UwamonoRecord *record;

    if ((gameWork.control & 0x20) != 0 && gameWork.field_1c != bestIndex) {
        return -1;
    }
    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        DoublewordVector4 spot;
        float dist;

        record = &((UwamonoRecord *) uwamonoRecords)[index];

        switch (record->kind) {
        case UWAMONO_KIND_SEARCHED:
            if ((unsigned int) (record->type - 4) < 3) {
                float angleMid;

                spot.words[0] = record->savedPositionXY;
                spot.words[1] = record->savedPositionZW;
                dist = Get_Distance(&subject->position, &spot.vector);
                if (bestDist <= dist) {
                    continue;
                }
                if (subject->position.y < ((Vector4 *) &record->savedPositionXY)->y - uwamonoRecords[uwamonoCurrentIndex].form) {
                    continue;
                }
                if (((Vector4 *) &record->savedPositionXY)->y + record->extentY < subject->position.y) {
                    continue;
                }
                angleMid = record->rotationY + 3.1415927f;
                if (unduCheckAngle(subject->heading, angleMid - 0.5235988f, angleMid + 0.5235988f) != 1) {
                    continue;
                }
                if ((unsigned int) (subject->actorKind + 0xFFFF0000) < 0x4000) {
                    if (unduCheckInsideFan(-1, &subject->position, subject->heading, 1.5f, &spot.vector, -1, 1.0471976f, 1.0f) == 0) {
                        continue;
                    }
                } else {
                    if (unduCheckInsideFan(-1, &subject->position, subject->heading, 7.5f, &spot.vector, -1, 1.0471976f, 1.0f) == 0) {
                        continue;
                    }
                }
                if (range < dist) {
                    continue;
                }
                bestDist = dist;
                bestIndex = index;
            } else {
                int linkedIndex;

                if (record->motionKind != 0 && record->moveFrame != -1) {
                    continue;
                }
                linkedIndex = record->linkIndex;
                if (linkedIndex == -1) {
                    spot.vector = record->statePosition[0];
                } else {
                    Get_MiddlePoint(&record->statePosition[0], &D_00340390[linkedIndex], 1, 2, &spot.vector);
                }
                dist = Get_Distance(&subject->position, &spot.vector);
                if (bestDist <= dist) {
                    continue;
                }
                if (subject->position.y < ((Vector4 *) &record->savedPositionXY)->y - uwamonoRecords[uwamonoCurrentIndex].form) {
                    continue;
                }
                if (((Vector4 *) &record->savedPositionXY)->y + record->extentY < subject->position.y) {
                    continue;
                }
                if (record->motionKind == 0) {
                    continue;
                }
                if (record->searchState == 2) {
                    if (record->reachRadius * 5.0f < dist) {
                        continue;
                    }
                } else {
                    if (record->reachRadius < dist) {
                        continue;
                    }
                }
                if (unduCheckInsideFan(-1, &subject->position, subject->heading, range, &spot.vector, -1, 1.0471976f, 1.0f) == 0) {
                    continue;
                }
                if (range < dist) {
                    continue;
                }
                bestDist = dist;
                bestIndex = index;
            }
            break;
        case UWAMONO_KIND_TALK:
            {
                float reachLimit;

                spot.words[0] = record->savedPositionXY;
                spot.words[1] = record->savedPositionZW;
                dist = subject->position.y - spot.vector.y;
                dist = dist * dist;
                if (dist > 4.0f) {
                    continue;
                }
                dist = Get_Distance(&subject->position, &spot.vector);
                if (bestDist <= dist) {
                    continue;
                }
                reachLimit = record->extentX + subject->subjectRadius + 0.1f;
                if ((record->flags & 0x800) == 0) {
                    reachLimit += 0.5f;
                }
                if (reachLimit < dist) {
                    continue;
                }
                if ((record->flags & 0x800) == 0) {
                    if (unduCheckInsideFan(-1, &subject->position, subject->heading, range, &spot.vector, -1, 1.5707964f, 1.0f) == 0) {
                        continue;
                    }
                }
                if (range < dist) {
                    continue;
                }
                bestDist = dist;
                bestIndex = index;
            }
            break;
        default:
            continue;
        }
    }
    return bestIndex;
}

/* Restores the record at `index`: clears flags bit 0x8, arms the 60-tick countdown and copies the saved position home. */
void uwamonoRestoreRecord(int index)
{
    UwamonoRecord *record = &((UwamonoRecord *) uwamonoRecords)[index];
    long long savedPositionXY = record->savedPositionXY;
    long long savedPositionZW = record->savedPositionZW;

    record->flags &= ~8;
    record->moveDuration = 0x3C;
    record->homePositionXY = savedPositionXY;
    record->homePositionZW = savedPositionZW;
    record->moveFrame = 0;
    seVoiceStartDefault(0x02000002);
}

void uwamonoReleaseRecord(int index);

typedef struct UwamonoTableEntry {
    short field_00;
    short type;
    short id;
    short count;
} UwamonoTableEntry;

extern UwamonoTableEntry *uwamonoEventTable;
extern char D_0069D428[];
extern char D_003E7270[];
void *func_A9B8D8(int id, int unused);
void *func_A9B7D8(int id, int unused);
void *func_A9B950(int id, int unused);
void *func_A9B9C8(int id, int unused);
int func_A9C7C8(int id);
int func_A9C828(int id);
int func_A9C8E8(int id);
void func_A9C888(int id);
void selectionBlockFill(void);
void msgChoiceShowPair(char *text, int count);
void uwamonoSendEvent(int a, int b, int c);
void func_002AA7F0(char *a, char *b);

void uwamonoUpdateCountdowns(void)
{
    UwamonoRecord *records;
    UwamonoRecord *record;
    UwamonoTableEntry *entry;
    Vector4 pose;
    char message[0x100];
    char *text = D_0069D428;
    int index;
    int count;
    int id;
    int repeat;
    short moveFrameCopy;
    int currentOffset;

    if ((gameWork.control & 1) != 0) {
        return;
    }
    records = (UwamonoRecord *) uwamonoRecords;
    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        record = &records[index];
        if (records[index].kind != UWAMONO_KIND_SEARCHED) {
            continue;
        }
        if (records[index].type < 12) {
            continue;
        }
        if ((records[index].flags & 0x02000000) != 0) {
            continue;
        }
        if (record->moveFrame == -1 && Get_Distance3D(&record->savedPositionXY, (long long *) (uwamonoCurrentIndex * sizeof(UwamonoRecord) + (int) &records->savedPositionXY)) <= records[uwamonoCurrentIndex].extentX) {
            uwamonoRestoreRecord(index);
        }
        if (record->moveFrame == -1) {
            continue;
        }

        moveFrameCopy = record->moveFrame;
        currentOffset = uwamonoCurrentIndex * sizeof(UwamonoRecord);
        pose.y = *(float *) (currentOffset + ((currentOffset & 0xFF) >> 8) + (int) records + 20) + 1.0f;
        pose.x = *(float *) (currentOffset + ((currentOffset & 0xFE) >> 8) + (int) records + 16);
        pose.z = *(float *) (currentOffset + ((currentOffset & 0xFC) >> 8) + (int) records + 24);
        pose.w = 1.0f;
        Get_MiddlePoint(&record->homePositionXY, &pose, record->moveFrame, record->moveDuration, &record->savedPositionXY);

        if (record->moveFrame == record->moveDuration / 2) {
            record->extent50.x = record->extent50.y = record->extent50.z = 1.0f;
            record->extent50.w = 1.0f;
            pose.x = pose.y = pose.z = 0.0f;
            pose.w = 1.0f;
            func_001F0750(index, &pose, record->moveFrame);
        }

        if (record->moveFrame == 1) {
            entry = (UwamonoTableEntry *) (record->tableIndex * 8 + (int) uwamonoEventTable);
            count = entry->count;
            id = entry->id;
            switch (entry->type) {
            case 1:
                text = func_A9B8D8(id, 0);
                repeat = 0;
                while (repeat < count) {
                    func_A9C7C8(id);
                    repeat++;
                }
                break;
            case 2:
                text = func_A9B7D8(id, 0);
                repeat = 0;
                while (repeat < count) {
                    func_A9C828(id);
                    repeat++;
                }
                break;
            case 3:
                text = func_A9B950(id, 0);
                repeat = 0;
                while (repeat < count) {
                    func_A9C8E8(id);
                    repeat++;
                }
                break;
            case 4:
                text = func_A9B9C8(id, 0);
                if (count > 0) {
                    repeat = count;
                    do {
                        func_A9C888(id);
                    } while (--repeat != 0);
                }
                selectionBlockFill();
                break;
            }
            gameWork.flags[record->tableIndex] = 1;
            {
                extern char D_0069D420[];
                D_0069D420[2] = (char) (count / 10 + 0x30);
                D_0069D420[3] = (char) (count % 10 + 0x30);
            }
            strcpy(message, text);
            {
                extern char D_0069D420[8];
                func_002AA7F0(message, D_0069D420);
            }
            repeat = D_003E7270[0];
            strcpy(D_003E7270, message);
            msgChoiceShowPair(text, count);
            uwamonoSendEvent(8, *(short *) record, -1);
        }

        if (record->moveFrame >= record->moveDuration) {
            record->moveFrame = -1;
            uwamonoReleaseRecord(index);
        }
    }
}

extern unsigned char D_0069D1E4;
extern float D_0069B05C;
extern float D_0069B060;
extern int D_002B5DA8[];

void sefSpawnEffectById(int effectId, Vector4 *position, Vector4 *outVector, Vector4 *scale, int count, float radius);
void vibrationStartFade(unsigned char targetValue, unsigned char currentValue, int framesTotal);
void uwamonoAddPoint(Vector4 *position, int key, float value);

void uwamonoTriggerRecord(int index)
{
    UwamonoRecord *record = &((UwamonoRecord *) uwamonoRecords)[index];
    Vector4 unit;
    DoublewordVector4 scale;
    Vector4 spawnOut;
    UwamonoRecord *other;
    struct XenoActor *actor;
    UwamonoRecord *partnerBase;
    UwamonoRecord *secondBase;
    int partnerOff;
    int secondOff;
    int farPartner;
    int partnerPosOff;
    long long *partnerPos;
    int partnerYOff;
    float *partnerY;
    int partnerScaleOff;
    float *partnerScale;
    int partnerFlagsOff;
    int *partnerFlags;
    int partnerFlagsClearOff;
    int *partnerFlagsClear;
    int secondPosOff;
    long long *secondPos;
    int secondYOff;
    float *secondY;
    int secondScaleOff;
    float *secondScale;
    int secondFlagsOff;
    int *secondFlags;
    int secondFlagsClearOff;
    int *secondFlagsClear;
    int isDefault;

    scale.words[0] = *(u64 *) &record->extentX;
    scale.words[1] = *(u64 *) &record->unmodeled_588[0];
    scale.vector.z = scale.vector.z + scale.vector.z;
    scale.vector.x = scale.vector.x + scale.vector.x;

    if (record->type == 2 || record->type == 3) {
        spawnOut.z = 5.0f;
        spawnOut.y = 5.0f;
        spawnOut.x = 5.0f;
        spawnOut.w = 1.0f;
        sefSpawnEffectById(record->effectId, (Vector4 *) &record->savedPositionXY, &spawnOut, &scale.vector, 30, record->rotationY);
    } else {
        spawnOut.z = 1.0f;
        spawnOut.y = 1.0f;
        spawnOut.x = 1.0f;
        spawnOut.w = 1.0f;
        sefSpawnEffectById(record->effectId, (Vector4 *) &record->savedPositionXY, &spawnOut, &scale.vector, 30, record->rotationY);
    }
    if (record->triggerSound != -1) {
        seVoiceStartDefault(record->triggerSound);
    }
    if (D_002B5DA8[0] != 3560) {
        vibrationStartFade(255, 32, 30);
    }
    if ((unsigned int) record->type < 4) {
        D_0069D1E4 = ((unsigned char *) &record->type)[0] & 1;
        switch (record->type) {
        case 0:
            uwamonoAddPoint((Vector4 *) &record->savedPositionXY, 300, 3.0f);
            break;
        case 1:
            uwamonoAddPoint((Vector4 *) &record->savedPositionXY, 300, 3.0f);
            break;
        case 2:
            uwamonoAddPoint((Vector4 *) &record->savedPositionXY, 300, 15.0f);
            break;
        case 3:
            uwamonoAddPoint((Vector4 *) &record->savedPositionXY, 300, 15.0f);
            break;
        }
    }

    if (record->linkB != -1) {
        farPartner = ((UwamonoRecord *) uwamonoRecords)[record->linkB].type >= 12;
        partnerPosOff = record->linkB * sizeof(UwamonoRecord) + 16;
        partnerPos = (long long *) ((char *) uwamonoRecords + partnerPosOff);
        partnerPos[0] = ((DoublewordVector4 *) &record->savedPositionXY)->words[0];
        partnerPos[1] = ((DoublewordVector4 *) &record->savedPositionXY)->words[1];
        partnerYOff = record->linkB * sizeof(UwamonoRecord) + 20;
        partnerY = (float *) ((char *) uwamonoRecords + partnerYOff);
        *partnerY += (farPartner == 1) ? 0.5f : 0.0f;
        unit.x = unit.y = unit.z = unit.w = 1.0f;
        partnerBase = (UwamonoRecord *) uwamonoRecords;
        partnerOff = record->linkB * sizeof(UwamonoRecord);
        *(float *) (partnerOff + ((partnerOff & 0xFE) >> 8) + (int) partnerBase + 80) = *(float *) (partnerOff + ((partnerOff & 0xFC) >> 8) + (int) partnerBase + 84) = *(float *) (partnerOff + (int) partnerBase + 88) = 0.0f;
        ((UwamonoRecord *) uwamonoRecords)[record->linkB].extent50.w = 1.0f;
        func_001F0750(record->linkB, &unit, (farPartner == 1) ? 20 : 0);
        partnerScaleOff = record->linkB * sizeof(UwamonoRecord) + 52;
        partnerScale = (float *) ((char *) uwamonoRecords + partnerScaleOff);
        *partnerScale = (farPartner == 1) ? D_0069B05C : 0.0f;
        uwamonoRecords[record->linkB].flags |= 2;
        uwamonoRecords[record->linkB].flags &= ~0x02000000;
    }

    if (record->linkA != -1) {
        other = &((UwamonoRecord *) uwamonoRecords)[record->linkA];
        secondPosOff = record->linkA * sizeof(UwamonoRecord) + 16;
        secondPos = (long long *) ((char *) uwamonoRecords + secondPosOff);
        secondPos[0] = ((DoublewordVector4 *) &record->savedPositionXY)->words[0];
        secondPos[1] = ((DoublewordVector4 *) &record->savedPositionXY)->words[1];
        actor = other->actor;
        ((DoublewordVector4 *) &actor->position)->words[0] = secondPos[0];
        ((DoublewordVector4 *) &actor->position)->words[1] = secondPos[1];
        unit.x = unit.y = unit.z = unit.w = 1.0f;
        secondYOff = record->linkA * sizeof(UwamonoRecord) + 20;
        secondY = (float *) ((char *) uwamonoRecords + secondYOff);
        *secondY += 0.0f;
        secondBase = (UwamonoRecord *) uwamonoRecords;
        secondOff = record->linkA * sizeof(UwamonoRecord);
        *(float *) (secondOff + ((secondOff & 0xFE) >> 8) + (int) secondBase + 80) = *(float *) (secondOff + ((secondOff & 0xFC) >> 8) + (int) secondBase + 84) = *(float *) (secondOff + (int) secondBase + 88) = 0.0f;
        ((UwamonoRecord *) uwamonoRecords)[record->linkA].extent50.w = 1.0f;
        func_001F0750(record->linkA, &unit, 20);
        ((UwamonoRecord *) uwamonoRecords)[record->linkA].scale34 = D_0069B060;
        uwamonoRecords[record->linkA].flags |= 2;
        uwamonoRecords[record->linkA].flags &= ~0x02000000;
        if (uwamonoRecords[uwamonoCurrentIndex].form != 5.0f) {
            sefSpawnEffectRecordById(0x1003400a, (Vector4 *) &record->savedPositionXY, 0);
        } else {
            sefSpawnEffectRecordById(0x1003400b, (Vector4 *) &record->savedPositionXY, 0);
        }
    }
    uwamonoReleaseRecord(index);
}


typedef struct UwamonoPoint {
    Vector4 position;                   /* +0x00: copied as one 16-byte block */
    short key;                          /* +0x10 */
    unsigned char unmodeled_12[2];
    float value;                        /* +0x14 */
    unsigned char unmodeled_18[8];
} UwamonoPoint;

extern UwamonoPoint uwamonoPoints[UWAMONO_POINT_COUNT];

/* Clears the 16 records: origin position with w = 1, key -1 (free), value 0. */
void uwamonoResetPoints(void)
{
    int index;

    for (index = 0; index < UWAMONO_POINT_COUNT; index++) {
        uwamonoPoints[index].position.x = uwamonoPoints[index].position.y = uwamonoPoints[index].position.z = 0.0f;
        uwamonoPoints[index].position.w = 1.0f;
        uwamonoPoints[index].key = -1;
        uwamonoPoints[index].value = 0.0f;
    }
}

/* Stores the position, value and key in the first free row (key -1); does nothing when no row is free. */
void uwamonoAddPoint(Vector4 *position, int key, float value)
{
    int index;

    for (index = 0; index < UWAMONO_POINT_COUNT; index++) {
        UwamonoPoint *row = &uwamonoPoints[index];

        if (row->key != -1) {
            continue;
        }
        row->key = key;
        row->position = *position;
        row->value = value;
        break;
    }
}

extern float D_0069B03C;
extern float D_0069B040;
extern float D_0069B044;
extern float D_0069B048;
extern float D_0069B04C;
extern float D_0069B050;
extern float D_0069B054;
extern float D_0069B058;
void uwamonoSetAction(int index, int action);

void uwamonoUpdatePoints(void)
{
    int pointIndex;
    int recordIndex;
    UwamonoPoint *point;
    UwamonoRecord *record;

    for (pointIndex = 0; pointIndex < UWAMONO_POINT_COUNT; pointIndex++) {
        point = &uwamonoPoints[pointIndex];
        if (point->key < 0) {
            continue;
        }
        for (recordIndex = 0; recordIndex < UWAMONO_RECORD_COUNT; recordIndex++) {
            UwamonoMotionScan *motion;
            record = &((UwamonoRecord *) uwamonoRecords)[recordIndex];
            motion = (struct UwamonoMotionScan *) record->motions;

            if (record->kind != UWAMONO_KIND_TALK) {
                continue;
            }
            if (motion->hold != 0) {
                continue;
            }
            if (point->value < Get_Distance3D(&point->position, &record->savedPositionXY)) {
                continue;
            }
            if (motion->actionState == 9) {
                continue;
            }
            if (motion->actionState != 6) {
                uwamonoSetAction(recordIndex, 9);
            }
        }
        point->key--;
    }
}

extern char D_0069D430[];
void xglFontDebugPrintf(int x, int y, const char *format, ...);

/* Scans the records for one with kind 2 and type in 7..11 (save points and dummies) that lies within half of the
 * current record's form from the current record; prints it and returns its low byte, else -1. */
int func_001FFB70(void)
{
    UwamonoRecord *record;
    int index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        record = &((UwamonoRecord *) uwamonoRecords)[index];
        if (record->kind == UWAMONO_KIND_SEARCHED && (unsigned int) (record->type - 7) < 5) {
            float distance = Get_Distance3D(&((UwamonoRecord *) uwamonoRecords)[uwamonoCurrentIndex].savedPositionXY, &record->savedPositionXY);
            if (distance <= 0.5f * uwamonoRecords[uwamonoCurrentIndex].form) {
                xglFontDebugPrintf(8, 8, D_0069D430, (double) distance);
                return (signed char) record->type;
            }
        }
    }
    return -1;
}
