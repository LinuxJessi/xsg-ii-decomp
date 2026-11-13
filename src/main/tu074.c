#include "common.h"
#include "shared.h"
#include "main/tu074.h"
/* Bytes per uwamonoSlotTable entry and per uwamonoRecords record. */
#define UWAMONO_RECORD_SIZE 0x870
/* Upper bound of the uwamonoSlotTable walk in disableDrawFlags. */
#define UWAMONO_SLOT_COUNT 0x100

/* Record kind byte (+0x02): 1 talks, 2 is searched. */
enum {
    UWAMONO_KIND_TALK = 1,
    UWAMONO_KIND_SEARCHED = 2
};

/* Record motion kind (+0x5C0): 1 moves between two points, anything else rotates. */
enum {
    UWAMONO_MOTION_MOVE = 1
};

typedef struct UwamonoActor UwamonoActor;

typedef struct UwamonoSlot UwamonoSlot;

/* One 0x870-byte slot of uwamonoSlotTable; the actor body pointer is at +0x00 and
   the kind byte of the same memory is at +0x542 of the slot before it. */
struct UwamonoSlot {
    UwamonoActor *actor; /* +0x00 */
    unsigned char unmodeled_04[0x542 - 0x04];
    unsigned char kind;  /* +0x542 */
    unsigned char unmodeled_543[0x870 - 0x543];
};

/* The actor body; the flags word at +0xBC is the one disableDrawFlags clears. */
struct UwamonoActor {
    unsigned char unmodeled_00[0xB0];
    float radius;      /* +0xB0: the record's extent x (uwamonoSetCollisionSize) */
    float extentCopyY; /* +0xB4: the record's extent y (uwamonoSetCollisionSize) */
    unsigned char unmodeled_b8[4];
    int flags; /* +0xBC */
};

extern UwamonoSlot uwamonoSlotTable[];
int func_A9F0A8(void);
void func_A9CC70(int source, int code);
int func_A9F0C0(int source, int which);
int func_A9F1C0(int source, int which);
int uwamonoFindIndexByShortKey(short key);

int disableDrawFlags(int source)
{
    int shortKey;
    int flagMask;
    int slotIndex;
    int kindFilter;
    int firstSlot;
    int endSlot;
    int slot;
    UwamonoActor *actor;

    if (func_A9F0A8() < 2) {
        func_A9CC70(source, 2);
        return 0;
    }
    shortKey = func_A9F0C0(source, 1);
    slotIndex = uwamonoFindIndexByShortKey(shortKey);
    if (slotIndex == -1) {
        return 1;
    }
    flagMask = func_A9F0C0(source, 2);
    if (func_A9F1C0(source, 3) != 0) {
        kindFilter = -1;
    } else {
        kindFilter = func_A9F0C0(source, 3);
    }
    if (kindFilter == -1) {
        firstSlot = slotIndex;
        endSlot = slotIndex + 1;
        if (slotIndex == kindFilter) {
            return 1;
        }
    } else {
        firstSlot = 0;
        endSlot = UWAMONO_SLOT_COUNT;
    }
    for (slot = firstSlot; slot < endSlot; slot++) {
        if (uwamonoSlotTable[slot - 1].kind != kindFilter && kindFilter != -1) {
            continue;
        }
        if (uwamonoSlotTable[slot - 1].kind != 4 && uwamonoSlotTable[slot - 1].kind != 1) {
            continue;
        }
        actor = uwamonoSlotTable[slot].actor;
        if (actor != 0) {
            actor->flags &= ~flagMask;
        }
    }
    return 1;
}

extern unsigned char D_003E6F58[];
extern unsigned char D_0069D308[];
void func_A9CDB0(void *, void *);

/* Registers the enepc native table D_003E6F58 with the module name "enepc". */
void enepcRegisterNatives(void) {
    func_A9CDB0(D_0069D308, D_003E6F58);
}

/* One 0x870-byte record of uwamonoRecords as this file reads it (tu051.h UwamonoObject is the shared view). */
typedef struct UwamonoRecordView {
    short id;                            /* +0x00 */
    unsigned char kind;                  /* +0x02 */
    unsigned char unmodeled_03;
    int key;                             /* +0x04 */
    int type;                            /* +0x08 */
    unsigned char unmodeled_0c[0x10 - 0x0C];
    AlignedVector4 position;             /* +0x10 */
    unsigned char unmodeled_20[0x40 - 0x20];
    AlignedVector4 rotation;               /* +0x40 */
    unsigned char unmodeled_50[0x25C - 0x50];
    short field25C;                      /* +0x25C */
    unsigned char unmodeled_25e[0x330 - 0x25E];
    UwamonoActor *actor;                /* +0x330: the record's actor */
    unsigned char unmodeled_334[0x570 - 0x334];
    int flags;                           /* +0x570 */
    unsigned char unmodeled_574[0x580 - 0x574];
    float extentX;                        /* +0x580: extent x, copied to the actor's radius */
    float extentY;                        /* +0x584: extent y */
    float extentZ;                        /* +0x588: extent z */
    float extentW;                        /* +0x58C: extent w */
    unsigned char searchMode;            /* +0x590: UwamonoSearch.mode in the shared header */
    unsigned char unmodeled_591[0x598 - 0x591];
    unsigned char searchFlag;            /* +0x598 */
    unsigned char unmodeled_599[0x59C - 0x599];
    int level;                           /* +0x59C */
    int effectId;                        /* +0x5A0 */
    int linkA;                           /* +0x5A4 */
    int linkB;                           /* +0x5A8 */
    unsigned char unmodeled_5ac[0x5B0 - 0x5AC];
    Vector4 focusOffset;                /* +0x5B0 */
    signed char motionKind;              /* +0x5C0: 1 = move between two points, else rotate */
    unsigned char searchState;              /* +0x5C1 */
    signed char busy;                    /* +0x5C2 */
    unsigned char unmodeled_5c3;
    short timer;                      /* +0x5C4 */
    unsigned char unmodeled_5c6[0x5D0 - 0x5C6];
    Vector4 closedPosition;             /* +0x5D0 */
    Vector4 openPosition;               /* +0x5E0 */
    Vector4 closedRotation;           /* +0x5F0 */
    Vector4 openRotation;             /* +0x600 */
    float speed;                        /* +0x610 */
    float swingLength;                      /* +0x614: swing length */
    int linkIndex;                         /* +0x618: index of the linked record, -1 = none */
    unsigned char unmodeled_61c[0x620 - 0x61C];
    int triggerSound;                        /* +0x620 */
    int moveSound;                        /* +0x624 */
    unsigned char unmodeled_628[0x634 - 0x628];
    int cellMask;                        /* +0x634 */
    unsigned char unmodeled_638[0x870 - 0x638];
} UwamonoRecordView;

extern UwamonoRecordView uwamonoRecords[];

extern float sinf(float angle);
extern float cosf(float angle);
void uwamonoClearMoveResetWord(int index);
void uwamonoAddMoveStep(int index, float a, float b);
void uwamonoClearRotateResetWord(int index);
void uwamonoAddRotateStep(int index, float a, float b);

/* The two doublewords of a vector the original copies with ld/sd. */
#define UWAMONO_WORDS(vector) (((DoublewordVector4 *) &(vector))->words)

int uwamonoSetDoorAll(int source)
{
    short suppliedCount = 0;
    int recordIndex;
    int shortKey;
    int argIndex;
    float motionDelta[3];
    float scaledValue;
    UwamonoRecordView *record;
    UwamonoRecordView *linkedRecord;

    if (func_A9F0A8() != 13) {
        func_A9CC70(source, 2);
        return 0;
    }
    shortKey = func_A9F0C0(source, 1);
    recordIndex = uwamonoFindIndexByShortKey(shortKey);
    record = &uwamonoRecords[recordIndex];
    if (record->linkIndex != -1) {
        linkedRecord = &uwamonoRecords[record->linkIndex];
    } else {
        linkedRecord = record;
    }

    for (argIndex = 1; argIndex < 13; argIndex++) {
        if (func_A9F1C0(source, argIndex) == 0) {
            suppliedCount++;
        }
    }
    if (suppliedCount == 2) {
        if (func_A9F1C0(source, 3) == 0) {
            int doorState = func_A9F0C0(source, 3);

            linkedRecord->timer = doorState;
            record->timer = doorState;
            return 1;
        }
        if (func_A9F1C0(source, 12) == 0) {
            int searchState = func_A9F0C0(source, 12);

            linkedRecord->searchState = searchState;
            record->searchState = searchState;
            return 1;
        }
    }

    {
        int motionKind;

        if (func_A9F1C0(source, 2) != 0) {
            motionKind = (unsigned char) record->motionKind;
        } else {
            motionKind = func_A9F0C0(source, 2);
        }
        record->motionKind = motionKind;
    }
    {
        int doorState;

        if (func_A9F1C0(source, 3) != 0) {
            doorState = (unsigned short) record->timer;
        } else {
            doorState = func_A9F0C0(source, 3);
        }
        linkedRecord->timer = doorState;
        record->timer = doorState;
    }

    if (func_A9F1C0(source, 8) != 0) {
        scaledValue = record->speed;
    } else {
        scaledValue = (float) func_A9F0C0(source, 8) / 100.0f;
    }
    linkedRecord->speed = scaledValue;
    record->speed = scaledValue;

    if (func_A9F1C0(source, 9) != 0) {
        scaledValue = record->extentY;
    } else {
        scaledValue = (float) func_A9F0C0(source, 9) / 100.0f;
    }
    record->extentY = scaledValue;

    {
        float extent;

        if (func_A9F1C0(source, 10) != 0) {
            extent = record->extentX;
        } else {
            extent = (float) func_A9F0C0(source, 10) / 100.0f * 0.5f;
        }
        record->extentX = extent;
    }

    if (func_A9F1C0(source, 11) != 0) {
        scaledValue = record->swingLength;
    } else {
        scaledValue = (float) func_A9F0C0(source, 11) / 100.0f;
    }
    record->swingLength = scaledValue;

    {
        int searchState;

        if (func_A9F1C0(source, 12) != 0) {
            searchState = record->searchState;
        } else {
            searchState = func_A9F0C0(source, 12);
        }
        linkedRecord->searchState = searchState;
        record->searchState = searchState;
    }
    {
        int value624;

        if (func_A9F1C0(source, 13) != 0) {
            value624 = record->moveSound;
        } else {
            value624 = func_A9F0C0(source, 13);
        }
        linkedRecord->moveSound = value624;
        record->moveSound = value624;
    }

    if (func_A9F1C0(source, 5) != 0 && func_A9F1C0(source, 6) != 0 && func_A9F1C0(source, 7) != 0) {
        float rotationY = record->rotation.y;
        float halfPi = 1.5707964f;
        double sine = sinf(rotationY + halfPi);
        double cosine;
        double reach = record->swingLength;

        if (reach < 0.0) {
            reach = -reach;
        }
        motionDelta[0] = sine * reach;
        motionDelta[1] = 0.0f;
        cosine = cosf(record->rotation.y + halfPi);
        reach = record->swingLength;
        if (reach < 0.0) {
            reach = -reach;
        }
        motionDelta[2] = cosine * reach;
    } else {
        motionDelta[0] = (float) func_A9F0C0(source, 5) / 100.0f;
        motionDelta[1] = (float) func_A9F0C0(source, 6) / 100.0f;
        motionDelta[2] = (float) func_A9F0C0(source, 7) / 100.0f;
    }

    record->busy = 1;
    record->field25C = -1;
    if (record->timer == 0 && record->linkIndex != -1) {
        uwamonoRecords[record->linkIndex].timer = 0;
    }

    if (record->motionKind == UWAMONO_MOTION_MOVE) {
        record->closedPosition.x = record->position.x;
        record->closedPosition.y = record->position.y;
        record->closedPosition.z = record->position.z;
        record->openPosition.x = record->position.x + motionDelta[0];
        record->openPosition.y = record->position.y + motionDelta[1];
        record->openPosition.z = record->position.z + motionDelta[2];
        uwamonoClearMoveResetWord(recordIndex);
        uwamonoAddMoveStep(recordIndex, 20.0f, 1.0f);
        uwamonoAddMoveStep(recordIndex, 5.0f, 0.5f);
        uwamonoAddMoveStep(recordIndex, 55.0f, 5.0f);
        uwamonoAddMoveStep(recordIndex, 5.0f, 3.0f);
        uwamonoAddMoveStep(recordIndex, 5.0f, 2.0f);
        uwamonoAddMoveStep(recordIndex, 5.0f, 1.0f);
        uwamonoAddMoveStep(recordIndex, 5.0f, 0.5f);
    } else {
        u64 copyWord;

        record->closedRotation.x = record->rotation.x;
        record->closedRotation.y = record->rotation.y;
        record->closedRotation.z = record->rotation.z;
        /* Both move points start at the position, copied as doublewords. Two
           statements are no-ops kept for the original store order: the low word
           is read back from the first copy (the index is always 0) and the high
           word is stored back to the position between the last two stores. */
        copyWord = UWAMONO_WORDS(record->position)[0];
        UWAMONO_WORDS(record->openPosition)[0] = copyWord;
        copyWord = UWAMONO_WORDS(record->openPosition)[(recordIndex & 0xFF) >> 8];
        UWAMONO_WORDS(record->closedPosition)[0] = copyWord;
        UWAMONO_WORDS(record->openPosition)[1] = UWAMONO_WORDS(record->position)[1];
        UWAMONO_WORDS(record->closedPosition)[1] = UWAMONO_WORDS(record->openPosition)[1];
        record->openRotation.x = record->rotation.x + (motionDelta[0] + motionDelta[0]);
        record->openRotation.y = record->rotation.y + (motionDelta[1] + motionDelta[1]);
        UWAMONO_WORDS(record->position)[1] = UWAMONO_WORDS(record->openPosition)[1];
        record->openRotation.z = record->rotation.z + (motionDelta[2] + motionDelta[2]);
        uwamonoClearRotateResetWord(recordIndex);
        uwamonoAddRotateStep(recordIndex, 20.0f, 1.0f);
        uwamonoAddRotateStep(recordIndex, 5.0f, 0.5f);
        uwamonoAddRotateStep(recordIndex, 55.0f, 5.0f);
        uwamonoAddRotateStep(recordIndex, 5.0f, 3.0f);
        uwamonoAddRotateStep(recordIndex, 5.0f, 2.0f);
        uwamonoAddRotateStep(recordIndex, 5.0f, 1.0f);
        uwamonoAddRotateStep(recordIndex, 5.0f, 0.5f);
    }
    return 1;
}

int func_A9F140(int source, int which);
int func_A9F238(int source, int value);
int srsEffectKeyFromName(int name);
int sefStartEffectKeyDefault(int first, int second);
void uwamonoStartPairMove(int index, int a, int b, int c);


int uwamonoSetKowaremonoAll(int source)
{
    int recordIndex;
    int shortKey;
    int value;
    float scale;
    int effectKey;
    int linkA;
    int linkB;
    UwamonoRecordView *record;

    if (func_A9F0A8() < 9) {
        func_A9CC70(source, 2);
        return 0;
    }
    shortKey = func_A9F0C0(source, 1);
    recordIndex = uwamonoFindIndexByShortKey(shortKey);
    {
        record = &uwamonoRecords[recordIndex];
        if (func_A9F1C0(source, 2) != 0) {
            value = record->searchMode;
        } else {
            value = func_A9F0C0(source, 2);
        }
        record->searchMode = value;
        if (func_A9F140(source, 3) == 3) {
            if (func_A9F1C0(source, 3) != 0) {
                effectKey = record->effectId;
            } else {
                effectKey = srsEffectKeyFromName(func_A9F238(source, func_A9F0C0(source, 3)));
            }
            record->effectId = effectKey;
        } else {
            if (func_A9F1C0(source, 3) != 0) {
                effectKey = record->effectId;
            } else {
                effectKey = func_A9F0C0(source, 3);
            }
            record->effectId = effectKey;
        }
        sefStartEffectKeyDefault(0, record->effectId);
        if (func_A9F1C0(source, 4) != 0) {
            value = record->searchFlag;
        } else {
            value = func_A9F0C0(source, 4);
        }
        record->searchFlag = value;
        if (func_A9F1C0(source, 5) != 0) {
            linkA = record->linkA;
        } else {
            linkA = uwamonoFindIndexByShortKey(func_A9F0C0(source, 5));
        }
        record->linkA = linkA;
        if (func_A9F1C0(source, 6) != 0) {
            linkB = record->linkB;
        } else {
            linkB = uwamonoFindIndexByShortKey(func_A9F0C0(source, 6));
        }
        record->linkB = linkB;
        if (func_A9F1C0(source, 7) != 0) {
            value = record->level;
        } else {
            value = func_A9F0C0(source, 7);
        }
        record->level = value;
        if (func_A9F1C0(source, 8) != 0) {
            value = record->triggerSound;
        } else {
            value = func_A9F0C0(source, 8);
        }
        record->triggerSound = value;
        if (func_A9F1C0(source, 9) != 0) {
            scale = record->focusOffset.x;
        } else {
            scale = (float) func_A9F0C0(source, 9) / 100.0f;
        }
        record->focusOffset.x = scale;
        if (func_A9F1C0(source, 10) != 0) {
            scale = record->focusOffset.y;
        } else {
            scale = (float) func_A9F0C0(source, 10) / 100.0f;
        }
        record->focusOffset.y = scale;
        if (func_A9F1C0(source, 11) != 0) {
            scale = record->focusOffset.z;
        } else {
            scale = (float) func_A9F0C0(source, 11) / 100.0f;
        }
        record->focusOffset.z = scale;
        if (func_A9F1C0(source, 12) == 0) {
            record->cellMask = 0;
            record->cellMask |= (func_A9F0C0(source, 12) & 1);
            record->cellMask |= (((func_A9F0C0(source, 12) / 10) & 1) * 2);
            record->cellMask |= (((func_A9F0C0(source, 12) / 100) & 1) * 4);
            record->cellMask |= (((func_A9F0C0(source, 12) / 1000) & 1) * 8);
            record->cellMask |= (((func_A9F0C0(source, 12) / 10000) & 1) * 0x10);
            record->cellMask |= (((func_A9F0C0(source, 12) / 100000) & 1) << 5);
            record->cellMask |= (((func_A9F0C0(source, 12) / 1000000) & 1) << 6);
            record->cellMask |= (((func_A9F0C0(source, 12) / 10000000) & 1) << 7);
        }
        while (record->linkB != -1) {
            uwamonoRecords[record->linkB].flags &= ~2;
            uwamonoRecords[record->linkB].flags |= 0x02000000;
            goto linkBDone;
        }
        if ((record->kind == UWAMONO_KIND_SEARCHED) && ((unsigned int) (record->type - 4) < 3U)) {
            uwamonoStartPairMove(recordIndex, 3, 1, record->linkB);
        }
linkBDone:
        if (record->linkA != -1) {
            uwamonoRecords[record->linkA].flags &= ~2;
            uwamonoRecords[record->linkA].flags |= 0x02000000;
        }
        return 1;
    }
}

void uwamonoSetPairMode(int, int);
void uwamonoTriggerRecord(int, int);
void uwamonoMoveSelection(int, int);

/*
 * Dispatches script argument 2 of this id to one of three helpers after the
 * index lookup on argument 1. Returns 1 when the call was handled.
 */
int uwamonoSendSignal(int source)
{
    int recordIndex;
    int signalId;

    if (func_A9F0A8() != 2) {
        func_A9CC70(source, 2);
        return 0;
    }
    recordIndex = uwamonoFindIndexByShortKey(func_A9F0C0(source, 1));
    signalId = func_A9F0C0(source, 2);
    recordIndex = recordIndex | ((signalId & 0xFF) >> 8);
    if (recordIndex != -1) {
        if ((unsigned int) signalId < 8) {
            switch (signalId) {
            case 0:
            case 1:
            case 2:
                uwamonoSetPairMode(recordIndex, signalId);
                break;
            case 3:
            case 4:
            case 5:
                break;
            case 6:
                uwamonoTriggerRecord(recordIndex, signalId);
                break;
            case 7:
                uwamonoMoveSelection(1, signalId);
                break;
            }
        }
        return 1;
    }
    return 0;
}

/* The record block at uwamonoScaleRecords: only the first float is read here.
   local view: the shared UwamonoScaleRecord of tu051.h cannot be included here, because tu051.h types uwamonoRecords as UwamonoObject and this file as UwamonoRecordView. */
typedef struct UwamonoScaleRecord {
    float scale;                        /* +0x000 */
    unsigned char unmodeled_004[0x870 - 0x004];
} UwamonoScaleRecord;

extern UwamonoScaleRecord uwamonoScaleRecords[];

typedef union WordUnion {
    float f;
    int i;
} WordUnion;

int uwamonoSetCollisionSize(int source)
{
    int recordIndex;
    int shortKey;
    int again;

    if (func_A9F0A8() == 4) {
        goto start;
    }
    func_A9CC70(source, 2);
    return 0;
    for (;;) {
    start:
    shortKey = func_A9F0C0(source, 1);
retry:
    again = 0;
    recordIndex = uwamonoFindIndexByShortKey(shortKey);
    {
        UwamonoScaleRecord *scaleRecord = &uwamonoScaleRecords[recordIndex];
        float scaleValue;

        if (func_A9F1C0(source, 2) != 0) {
            scaleValue = scaleRecord->scale;
        } else {
            float factor = (float) func_A9F0C0(source, 2);

            scaleValue = factor / 100.0f * 0.5f;
        }
        scaleRecord->scale = scaleValue;
    }
    {
        int extentYOffset = 0x584 + recordIndex * UWAMONO_RECORD_SIZE;
        float *extentY = (float *) ((unsigned char *) uwamonoRecords + extentYOffset);
        float value;

        if (func_A9F1C0(source, 3) != 0) {
            value = *extentY;
        } else {
            value = (float) func_A9F0C0(source, 3) / 100.0f;
        }
        *extentY = value;
    }
    {
        int extentZOffset = recordIndex * UWAMONO_RECORD_SIZE + 0x588;
        float *extentZ = (float *) ((unsigned char *) uwamonoRecords + extentZOffset);
        float value;

        if (func_A9F1C0(source, 4) != 0) {
            value = *extentZ;
        } else {
            float factor = (float) func_A9F0C0(source, 4);

            value = factor / 100.0f * 0.5f;
        }
        *extentZ = value;
    }
    {
        int extentWOffset = recordIndex * UWAMONO_RECORD_SIZE + 0x58C;
        WordUnion *word = (WordUnion *) ((unsigned char *) uwamonoRecords + extentWOffset);
        UwamonoRecordView *record = &uwamonoRecords[recordIndex];
        UwamonoActor *actorBody;

        word->f = 1.0f;
        actorBody = uwamonoRecords[recordIndex].actor;
        if (actorBody != 0) {
            actorBody->radius = uwamonoRecords[recordIndex].extentX;
            actorBody->extentCopyY = record->extentY;
        }
    }
    if (again) {
        goto retry;
    }
    return 1;
    }
}

int setCollisionScale(int source)
{
    int recordIndex;
    UwamonoRecordView *records;

    if (func_A9F0A8() != 4) {
        func_A9CC70(source, 2);
        return 0;
    }
    records = uwamonoRecords;
    recordIndex = uwamonoFindIndexByShortKey(func_A9F0C0(source, 1));
    {
        UwamonoScaleRecord *scaleRecord = &uwamonoScaleRecords[recordIndex];
        float value;

        if (func_A9F1C0(source, 2) != 0) {
            value = scaleRecord->scale;
        } else {
            float factor = (float) func_A9F0C0(source, 2);

            value = scaleRecord->scale * factor / 100.0f;
        }
        scaleRecord->scale = value;
    }
    {
        unsigned int extentYOffset = recordIndex * UWAMONO_RECORD_SIZE + 0x584;
        float *extentY = (float *) ((unsigned char *) uwamonoRecords + extentYOffset);
        float value;

        if (func_A9F1C0(source, 3) != 0) {
            value = *extentY;
        } else {
            value = *extentY * (float) func_A9F0C0(source, 3) / 100.0f;
        }
        *extentY = value;
    }
    {
        unsigned int extentZOffset = recordIndex * UWAMONO_RECORD_SIZE + 0x588;
        float *extentZ = (float *) ((unsigned char *) uwamonoRecords + extentZOffset);
        float value;

        if (func_A9F1C0(source, 4) != 0) {
            value = *extentZ;
        } else {
            value = *extentZ * (float) func_A9F0C0(source, 4) / 100.0f;
        }
        *extentZ = value;
    }
    {
        UwamonoActor *actorBody;

        records = &uwamonoRecords[recordIndex];
        records->extentW = 1.0f;
        do {
            actorBody = uwamonoRecords[recordIndex].actor;
            if (actorBody != 0) {
                actorBody->radius = uwamonoRecords[recordIndex].extentX;
            }
        } while (0);
    }
    return 1;
}
