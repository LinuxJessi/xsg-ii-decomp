#include "common.h"
#include "main/TiDrawCollision.h"
#include "main/TiMRPackage.h"
#include "main/TiMtnBlend.h"
#include "main/test_128.h"
#include "main/tu026.h"
#include "main/xenoActor.h"

/* XenoActorPoolGet: pool record at index (records are 0x7F0 bytes apart), or 0 outside XenoActorPoolCount. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorPoolGet);

/* XenoActorPoolInit: sets the pool base and count, builds the active table after the pool and inits every record. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorPoolInit);

#include "shared.h"

typedef struct XenoActor {
    unsigned char unmodeled_000[0x10];
    float rowA[3];                          /* 0x010 */
    unsigned char unmodeled_01c[0x50 - 0x1C];
    float rowB[3];                          /* 0x050 */
    unsigned char unmodeled_05c[0x80 - 0x5C];
    unsigned char block80[0x10];            /* 0x080 */
    float rowC[3];                          /* 0x090 */
    unsigned char unmodeled_09c[0x100 - 0x9C];
    float localMatrix[4][4];                /* 0x100 */
    unsigned char unmodeled_140[0x5A0 - 0x140];
    void *overrideMatrix;                   /* 0x5A0 */
    unsigned char unmodeled_5a4[0x5A8 - 0x5A4];
    int slotIndex;                          /* 0x5A8 */
    unsigned char unmodeled_5ac[0x5CC - 0x5AC];
    u32 flags;                              /* 0x5CC: func_0028AF28 sets bit 0x400; XenoActorSetSelector sets 0x10 */
    float transformScale;                   /* 0x5D0: XenoActorUpdateLinkMatrix passes it as the scale; XenoActorApplySourceValue stores source_value here */
    int mode;                               /* 0x5D4: XenoActorCheckMode compares it with its argument */
    unsigned char unmodeled_5d8[0x5DC - 0x5D8];
    int requested_mode;                     /* 0x5DC: XenoActorRequestMode stores its argument here */
    float cachedValue;                      /* 0x5E0 */
    int cachedSelector;                     /* 0x5E4 */
    unsigned char unmodeled_5e8[0x620 - 0x5E8];
    int link;                               /* 0x620 */
    unsigned char unmodeled_624[0x640 - 0x624];
    int handle640[4];                       /* 0x640: its address is passed to callees (the 0x60-byte channel of XenoActorSetSelector); element 1 is stored by func_0028B2F0 */
    int flags650;                           /* 0x650 */
    unsigned char unmodeled_654[0x660 - 0x654];
    int source_value;                       /* 0x660 */
    unsigned char unmodeled_664[0x6A0 - 0x664];
    int flags6A0;                           /* 0x6A0: the channel flags; XenoActorSetSelector gates its setup on bit 0x2 */
    unsigned char unmodeled_6A4[0x708 - 0x6A4];
    struct XenoActorEntry *entry;           /* 0x708 */
    unsigned char unmodeled_70C[0x710 - 0x70C];
    struct XenoActorTable *table;           /* 0x710 */
    unsigned char unmodeled_714[0x7A4 - 0x714];
    short activeItem;                       /* 0x7A4: item index XenoActorRefreshItems compares against its loop counter */
} XenoActor;

void XenoActorPoolInit(void *data, int size);
extern u8 D_0088AEF0[];

/* XenoActorPoolInitDefault: initializes the pool from the built-in buffer D_0088AEF0 (0x30 records). */
void XenoActorPoolInitDefault(void) {
    XenoActorPoolInit(D_0088AEF0, 0x30);
}

extern unsigned char xenoActorAssertFile[];
extern unsigned char xenoActorAssertExpr[];
extern int XenoActorPoolCount;
extern int *XenoActorActiveTable;
int InstanceOfRgHeapById(int);
void tiMRStateSetValue(void *, float);
void XenoActorRelease(int);
int func_0029A6D0(int);
int func_0029A980(int);
void XenoActorPushMatrix(void);
void xglCurMatrixSet(f32 *);
void xglCurApplyVector(Vector4 *, const Vector4 *);
void xacEvalTransform(float *, int *, float);
void XenoActorAttachChild(int, struct XenoActor *);
void xglMatrixMul(void *, void *, void *);
void xglVectorMulMat(Vector4 *, float (*)[4], const Vector4 *);

/* XenoActorInitHeap: initializes RG heap 5, the heap the actor matrices are allocated from, over the given memory block. */
void XenoActorInitHeap(int memory, int size) {
    InitRgHeap(InstanceOfRgHeapById(5), memory, size);
}

/* XenoActorInit: resets one record to its default state (slotIndex and mode -1, default channel values). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorInit);

/* XenoActorAlloc: takes the first free record from the pool, initializes it and adds it to the active table. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorAlloc);

/* XenoActorAllocMatrices: frees and reallocates the matrix handles of a record from its resource at 0x5F4. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorAllocMatrices);

/* XenoActorAttachChild: links a live child under a parent's child slots (0x5AC) and sets the child parent (0x5A4). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorAttachChild);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorAppendChildOnJoint);

/* XenoActorAttachChildToJoint: finds the joint with the given id in the resource joint table and attaches the child there. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorAttachChildToJoint);

/* XenoActorAttachAtSlot: stores the slot index, attaches the child to parent and returns 1; 0 on bad arguments. */
int XenoActorAttachAtSlot(int parent, struct XenoActor *actor, int slotIndex) {
    if (parent != 0 && actor != 0 && slotIndex >= 0) {
        actor->slotIndex = slotIndex;
        XenoActorAttachChild(parent, actor);
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorReleaseChild);

/* XenoActorSetFlagAll: ORs a flag mask into every live record. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorSetFlagAll);

void XenoActorSetChannelValue(struct XenoActor *actor, float value) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x19F);
    }
    tiMRStateSetValue(&actor->flags650, value);
}

void XenoActorSetChannelValueB(struct XenoActor *actor, float value) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1A9);
    }
    tiMRStateSetValue(&actor->flags6A0, value);
}

void XenoActorEnableChannel(struct XenoActor *actor) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1B5);
    }
    tiWordSetBits(&actor->flags650, 4);
    tiWordSetBits(&actor->flags650, 2);
}

void func_002885D8(struct XenoActor *actor) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1C2);
    }
    tiWordClearBits(&actor->flags650, 4);
    tiWordSetBits(&actor->flags650, 2);
}

void XenoActorEnableChannelB(struct XenoActor *actor) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1CD);
    }
    tiWordSetBits(&actor->flags6A0, 4);
    tiWordSetBits(&actor->flags6A0, 2);
}

void func_00288678(struct XenoActor *actor) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1D8);
    }
    tiWordClearBits(&actor->flags6A0, 4);
    tiWordSetBits(&actor->flags6A0, 2);
}

void XenoActorResetChannelFlags(struct XenoActor *actor) {
    tiWordClearBits(&actor->flags650, 2);
    tiWordClearBits(&actor->flags650, 0x40);
    tiWordClearBits(&actor->flags6A0, 2);
    tiWordClearBits(&actor->flags6A0, 0x40);
}

void tiMRPackageSetEntryA(void *child, int flags);
void tiMRPackageSetTableA(void *child, int flags);

void XenoActorClearChannel(XenoActor *actor) {
    tiMRPackageSetTableA(actor->handle640, 0);
    tiMRPackageSetEntryA(actor->handle640, 0);
}

/* XenoActorSetMode: sets the current mode and slot, resets requested_mode and re-inits channel 0x640. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorSetMode);

void XenoActorSetMode(void);

void XenoActorCheckMode(XenoActor *actor, int expected_mode) {
    if (actor->mode != expected_mode) {
        XenoActorSetMode();
    }
}

/* XenoActorSetPlayRange: sets the playback range and clamps the playback value into it. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorSetPlayRange);

void XenoActorConvertSourceRecord(void *child);

void XenoActorApplySourceValue(XenoActor *actor) {
    actor->transformScale = (f32) actor->source_value;
    XenoActorConvertSourceRecord(&actor->flags650);
}

/* XenoActorSetModeInSlot: like XenoActorSetMode, for one of the four mode slots (asserts slot < 4). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorSetModeInSlot);

/*
 * Partial view of the xenoActor record: only the offsets this TU's claimed
 * functions touch are named (XenoActorSetSelector at +0x5CC..+0x7A4 and
 * XenoActorRefreshItems at +0x710 and +0x7A4). Gaps are unmodeled spans.
 */
typedef struct XenoActorEntry {
    unsigned char unmodeled_00[0x08];
    f32 value;                      /* +0x08 */
} XenoActorEntry;

typedef struct XenoActorTable {
    unsigned char unmodeled_00[0x3C];
    u32 count;                      /* +0x3C */
    unsigned char unmodeled_40[0x4C - 0x40];
    int dataOffset;                 /* +0x4C: bytes from the table to its items */
} XenoActorTable;

typedef struct XenoActorItem {
    unsigned char unmodeled_00[0x04];
    f32 weight;                     /* +0x04 */
    unsigned char unmodeled_08[0x20 - 0x08];
    unsigned char payload[0x10];    /* +0x20: passed by address */
} XenoActorItem;

f32 tiMRStateSetPosition(void *channelFlags, f32 value);
f32 tiMRPackageSelectEntryB(void *channel, int selector);
f32 tiMtnBlendSwapGlobal(void *channel, void *channelFlags);

/* XenoActorSetSelector: stores the selector, reads the channel value and copies the entry value into cachedValue. */
f32 XenoActorSetSelector(XenoActor *actor, int selector)
{
    XenoActorEntry *entry;
    f32 result;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 637);
    }
    actor->cachedSelector = selector;
    result = tiMRPackageSelectEntryB(actor->handle640, selector);
    entry = actor->entry;
    if (entry != 0) {
        f32 entryValue = entry->value;

        actor->cachedValue = entryValue;
        result = tiMRStateSetPosition(&actor->flags6A0, entryValue);
        if (!(actor->flags & 0x10) && (actor->flags6A0 & 2)) {
            tiMtnBlendSwapLocal(actor->handle640, &actor->flags6A0);
            result = tiMtnBlendSwapGlobal(actor->handle640, &actor->flags6A0);
            actor->flags |= 0x10;
        }
    }
    return result;
}

void XenoActorRequestMode(struct XenoActor *actor, int mode) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x29A);
    }
    actor->requested_mode = mode;
}

int XenoActorIsDone(struct XenoActor *actor) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x2A5);
    }
    if ((actor->flags650 & 8) != 0) {
        return 1;
    }
    return 0;
}

/* XenoActorUpdateLinkMatrix: when the actor has a link, rebuilds its rowA..rowC block from the link's joint row. */
void XenoActorUpdateLinkMatrix(struct XenoActor *actor) {
    float block[9];
    int linkRow;
    int link;

    link = actor->link;
    if (link != 0) {
        linkRow = link + 0x50;
        xacEvalTransform(block, &linkRow, actor->transformScale);
        actor->rowA[0] = block[0];
        actor->rowA[1] = block[1];
        actor->rowA[2] = block[2];
        actor->rowB[0] = block[3];
        actor->rowB[1] = block[4];
        actor->rowB[2] = block[5];
        actor->rowC[0] = block[6];
        actor->rowC[1] = block[7];
        actor->rowC[2] = block[8];
    }
}

/* XenoActorApplyRequestedMode: applies a pending mode request, then updates channel 0x640. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorApplyRequestedMode);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", func_00288E08);

extern int XenoActorHoldFlag;

/* XenoActorSetHoldFlag: sets the global update hold XenoActorHoldFlag; XenoActorDoit skips every tree update while it is set. */
void XenoActorSetHoldFlag(void) {
    XenoActorHoldFlag = 1;
}

/* XenoActorClearHoldFlag: clears the global update hold XenoActorHoldFlag; XenoActorPoolInit clears it when the pool is built. */
void XenoActorClearHoldFlag(void) {
    XenoActorHoldFlag = 0;
}

/* XenoActorRelease: frees the matrix handles, unlinks the record from its parent and the active table, then resets it. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorRelease);

/* XenoActorUpdateMoveState: state-driven movement (state at +0x160, angle +0x150, velocity +0x154). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", func_00289028);

/* XenoActorUpdateCameraFade: fades the actor at +0x188 by its distance to the active camera. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorUpdateCameraFade);

/* XenoActorUpdateTree: per-frame update of the actor and its child records (recursive). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorUpdateTree);

extern unsigned char XenoActorItemTextIdle[];
extern unsigned char XenoActorItemTextActive[];

/* XenoActorRefreshItems: draws each table item; the active item uses its own text. */
void XenoActorRefreshItems(XenoActor *actor)
{
    XenoActorTable *table;
    XenoActorItem *item;
    u32 itemIndex;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 1326);
    }
    table = actor->table;
    itemIndex = 0;
    if (table != 0) {
        item = (XenoActorItem *) ((unsigned char *) table + table->dataOffset);
        if (table->count != 0) {
            do {
                f32 weight = item->weight;

                if (actor->activeItem == itemIndex) {
                    tiDrawCollisionUpdate(item->payload, XenoActorItemTextActive, weight);
                } else {
                    tiDrawCollisionUpdate(item->payload, XenoActorItemTextIdle, weight);
                }
                itemIndex++;
                item++;
            } while (itemIndex < actor->table->count);
        }
        actor->activeItem = -1;
    }
}

/* XenoActorMulMatrix: multiplies a matrix by the actor override or local matrix when its channel is active. */
int XenoActorMulMatrix(struct XenoActor *actor, int unusedArg, void *matrix) {
    int result;

    result = func_0029A6D0((int) actor->handle640);
    if (result != 0) {
        if (actor->overrideMatrix != 0) {
            xglMatrixMul(matrix, matrix, actor->overrideMatrix);
        } else {
            xglMatrixMul(matrix, matrix, actor->localMatrix);
        }
        result = 1;
    }
    return result;
}

/* XenoActorMulVector: multiplies a vector by the actor override or local matrix when its channel is active. */
int XenoActorMulVector(struct XenoActor *actor, int unusedArg, Vector4 *vector) {
    int result;

    result = func_0029A980((int) actor->handle640);
    if (result != 0) {
        if (actor->overrideMatrix != 0) {
            xglVectorMulMat(vector, actor->overrideMatrix, vector);
        } else {
            xglVectorMulMat(vector, actor->localMatrix, vector);
        }
        result = 1;
    }
    return result;
}

/* XenoActorPushMatrix: pushes the actor transform on the matrix stack (rotations at 0x50..0x58, offsets 0x70 and 0x90). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorPushMatrix);

void tiMRPackageSetFirstWord(void *child);

void XenoActorSetFirstWord(XenoActor *actor) {
    tiMRPackageSetFirstWord(actor->handle640);
}

void tiMRPackageSetVectorUniform(void *child);

void XenoActorSetVectorUniform(XenoActor *actor) {
    tiMRPackageSetVectorUniform(actor->handle640);
}

void tiMRPackageSetVector(void *child, float x, float y, float z);

void XenoActorSetVector(XenoActor *actor, float x, float y, float z) {
    tiMRPackageSetVector(actor->handle640, x, y, z);
}

/* XenoActorDrawMotion: draws the motion pack (HRC, global matrices and PAM data) of the actor. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorDrawMotion);

/* XenoActorReleaseAll: releases every record in the active table. */
void XenoActorReleaseAll(void) {
    int index;
    int activeRecord;

    index = 0;
    if (XenoActorPoolCount > 0) {
        do {
            activeRecord = XenoActorActiveTable[index];
            index += 1;
            if (activeRecord != 0) {
                XenoActorRelease(activeRecord);
            }
        } while (index < XenoActorPoolCount);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorDoit);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorDisp);

/* XenoActorFindPart: index of the part with the given name (strcmp search). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorFindPart);

/* XenoActorHidePart: hides a named part (sets its bit in the visibility word). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorHidePart);

/* XenoActorShowPart: shows a named part (clears its bit in the visibility word). */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorShowPart);

void func_0028AF28(XenoActor *actor) {
    actor->flags |= 0x400;
}

/* XenoActorShiftJoints: adds or subtracts the actor offset to the translation of every joint. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorShiftJoints);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", func_0028B060);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", func_0028B158);

INCLUDE_ASM("asm/main/nonmatchings/xenoActor", func_0028B268);

void func_0028B2F0(XenoActor *actor, int value) {
    actor->handle640[1] = value;
}

/* XenoActorGetSlotRange: the two bounds of an entry in slot table 0x5FC[slot]. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorGetSlotRange);

/* XenoActorGetPoolCount: returns the pool count XenoActorPoolCount. */
int XenoActorGetPoolCount(void) {
    return XenoActorPoolCount;
}

void XenoActorTransformOffset(struct XenoActor *actor, int id) {
    XenoActorPushMatrix();
    xglCurMatrixSet(actor->localMatrix);
    xglCurApplyVector(id, actor->block80);
}
