#include "common.h"
#include "main/TiDrawCollision.h"
#include "main/TiMRPackage.h"
#include "main/TiMtnBlend.h"
#include "main/nml_model_set.h"
#include "main/test_128.h"
#include "main/tu026.h"
#include "main/xenoActor.h"

/* XenoActorPoolGet: pool record at index (records are 0x7F0 bytes apart), or 0 outside XenoActorPoolCount. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorPoolGet);

/* XenoActorPoolInit: sets the pool base and count, builds the active table after the pool and inits every record. */
INCLUDE_ASM("asm/main/nonmatchings/xenoActor", XenoActorPoolInit);

#include "shared.h"

typedef struct XenoMatrix {
    float m[4][4];
} XenoMatrix;

/* Header of a PXY motion pack (actor->pack). */
typedef struct PxyHeader {
    char magic[4];
    float version;
    u32 flags;                              /* 0x08: bit 0x2000 = per-geometry shadow mask */
    unsigned char unmodeled_0c[0x54 - 0xC];
    int rangeOffset;                        /* 0x54: bytes from the pack to its PxyRange */
    unsigned char unmodeled_58[4];
    int jointMatrixCount;                   /* 0x5C */
    int geometryCount;                      /* 0x60 */
    int jointMatrixOffset;                  /* 0x64: bytes from the pack to its joint matrices */
    unsigned char unmodeled_68[4];
    int geometryOffset;                     /* 0x6C: bytes from the pack to its PxyGeometry array */
    unsigned char unmodeled_70[0x8C - 0x70];
    float radius;                           /* 0x8C */
    unsigned char unmodeled_90[0xD0 - 0x90];
    int subPackOffset;                      /* 0xD0: bytes from the pack to a second pack, 0 if none */
} PxyHeader;

typedef struct PxyRange {
    unsigned char unmodeled_00[0x18];
    int zwrite;                             /* 0x18 */
} PxyRange;

typedef struct PxyGeometry {
    unsigned char unmodeled_00[0x20];
    u32 flags;                              /* 0x20: bit 0x100000 = casts a shadow */
    unsigned char unmodeled_24[0x70 - 0x24];
} PxyGeometry;

/* Motion hierarchy (HRC) of an actor (actor->table). */
typedef struct XenoHrc {
    unsigned char unmodeled_00[0xC];
    u32 numTransform;                       /* 0x0C */
    int index10;                            /* 0x10: the three indices sum to the focus joint */
    int index14;                            /* 0x14 */
    int index18;                            /* 0x18 */
    int focus;                              /* 0x1C */
    unsigned char unmodeled_20[0x3C - 0x20];
    u32 count;                              /* 0x3C: item count */
    unsigned char unmodeled_40[0x4C - 0x40];
    int dataOffset;                         /* 0x4C: bytes from the table to its items */
    unsigned char unmodeled_50[0x5C - 0x50];
    u32 flags;                              /* 0x5C: bit 4 = has a focus joint */
} XenoHrc;

typedef struct XenoLight {
    unsigned char unmodeled_000[0x12C];
    float intensity;                        /* 0x12C */
    unsigned char unmodeled_130[0x180 - 0x130];
} XenoLight;

typedef struct XenoActor {
    unsigned short attr;                    /* 0x000: bit 2 = has global matrices */
    unsigned char unmodeled_002[0x10 - 0x2];
    float rowA[3];                          /* 0x010 */
    unsigned char unmodeled_01c[0x50 - 0x1C];
    float rowB[3];                          /* 0x050 */
    unsigned char unmodeled_05c[0x80 - 0x5C];
    DoublewordAlignedVector4 position;      /* 0x080 */
    float rowC[3];                          /* 0x090 */
    unsigned char unmodeled_09c[0xBC - 0x9C];
    u32 drawFlags;                          /* 0x0BC */
    float alpha;                            /* 0x0C0 */
    int drawMode;                           /* 0x0C4 */
    unsigned char unmodeled_0c8[4];
    float modeValue;                        /* 0x0CC */
    int renderFlags;                        /* 0x0D0 */
    float renderValue;                      /* 0x0D4 */
    unsigned char unmodeled_0d8[4];
    unsigned char lightSource[0x100 - 0xDC]; /* 0x0DC */
    XenoMatrix localMatrix;                 /* 0x100 */
    float focusX;                           /* 0x140 */
    float focusY;                           /* 0x144 */
    float modeValue2;                       /* 0x148 */
    float modeValue3;                       /* 0x14C */
    unsigned char unmodeled_150[0x158 - 0x150];
    float shadowValue;                      /* 0x158 */
    int shadowCount;                        /* 0x15C */
    unsigned char unmodeled_160[4];
    float shadowValue2;                     /* 0x164 */
    float shadowValue3;                     /* 0x168 */
    float heightOffset;                     /* 0x16C */
    float fogA;                             /* 0x170 */
    float fogB;                             /* 0x174 */
    float fogC;                             /* 0x178 */
    float fogD;                             /* 0x17C */
    float modeValue4;                       /* 0x180 */
    unsigned char unmodeled_184[4];
    float alphaScale;                       /* 0x188 */
    unsigned char unmodeled_18c[4];
    XenoLight light;                        /* 0x190 */
    XenoLight sourceLight;                  /* 0x310 */
    float color[4];                         /* 0x490 */
    unsigned char colorBlock[0x10];         /* 0x4A0 */
    unsigned char sourceBlock[0x10];        /* 0x4B0 */
    unsigned char visibility[0x20];         /* 0x4C0 */
    unsigned char block4E0[0x20];           /* 0x4E0 */
    u32 geometryMask[8];                    /* 0x500: one bit per shadow-casting geometry */
    unsigned char block520[0x20];           /* 0x520 */
    unsigned char shadowA[0x10];            /* 0x540 */
    unsigned char shadowA2[0x10];           /* 0x550 */
    unsigned char shadowB[0x10];            /* 0x560 */
    unsigned char shadowB2[0x10];           /* 0x570 */
    unsigned char shadowC[0x10];            /* 0x580 */
    unsigned char colorIndex;               /* 0x590 */
    unsigned char unmodeled_591[0x5A0 - 0x591];
    void *overrideMatrix;                   /* 0x5A0 */
    struct XenoActor *parent;               /* 0x5A4 */
    int slotIndex;                          /* 0x5A8 */
    struct XenoActor *children[8];          /* 0x5AC */
    u32 flags;                              /* 0x5CC: func_0028AF28 sets bit 0x400; XenoActorSetSelector sets 0x10 */
    float transformScale;                   /* 0x5D0: XenoActorUpdateLinkMatrix passes it as the scale; XenoActorApplySourceValue stores source_value here */
    int mode;                               /* 0x5D4: XenoActorCheckMode compares it with its argument */
    unsigned char unmodeled_5d8[0x5DC - 0x5D8];
    int requested_mode;                     /* 0x5DC: XenoActorRequestMode stores its argument here */
    float cachedValue;                      /* 0x5E0 */
    int cachedSelector;                     /* 0x5E4 */
    unsigned char unmodeled_5e8[0x5EC - 0x5E8];
    PxyHeader *pack;                        /* 0x5EC */
    int effect;                             /* 0x5F0 */
    unsigned char unmodeled_5f4[0x614 - 0x5F4];
    int pamActive;                          /* 0x614 */
    unsigned char unmodeled_618[0x620 - 0x618];
    int link;                               /* 0x620 */
    unsigned char unmodeled_624[0x634 - 0x624];
    int pamFrame;                           /* 0x634 */
    unsigned char unmodeled_638[0x640 - 0x638];
    int handle640[4];                       /* 0x640: its address is passed to callees (the 0x60-byte channel of XenoActorSetSelector); element 1 is stored by func_0028B2F0 */
    int flags650;                           /* 0x650 */
    unsigned char unmodeled_654[0x660 - 0x654];
    int source_value;                       /* 0x660 */
    unsigned char unmodeled_664[0x6A0 - 0x664];
    int flags6A0;                           /* 0x6A0: the channel flags; XenoActorSetSelector gates its setup on bit 0x2 */
    unsigned char unmodeled_6A4[0x700 - 0x6A4];
    struct XenoActorEntry *motionKeys[4];   /* 0x700: one per mode slot; XenoActorSetSelector reads slot 2 */
    XenoHrc *table;                         /* 0x710 */
    void *pamData;                          /* 0x714 */
    unsigned char unmodeled_718[4];
    XenoMatrix *matrices;                   /* 0x71C */
    XenoMatrix *globalMatrices;             /* 0x720 */
    unsigned char unmodeled_724[0x79C - 0x724];
    int state;                              /* 0x79C: tu248 writes it; XenoActorDrawMotion reads and clears it */
    unsigned char unmodeled_7a0[0x7A4 - 0x7A0];
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
void XenoActorPushMatrix(XenoActor *actor);
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
    entry = actor->motionKeys[2];
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
    XenoHrc *table;
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
            xglMatrixMul(matrix, matrix, &actor->localMatrix);
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
            xglVectorMulMat(vector, actor->localMatrix.m, vector);
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

extern char D_0068D7A0[], D_0068D7C0[], D_0068D7E0[], D_0068D828[];
void func_00295EE8(XenoActor *actor, XenoMatrix *global, XenoMatrix *local);
void func_001BAA98(int state);
void pxySetModelScale(float *row);
void func_001BA248(unsigned char *color);
void nmlModelSetZwrite(int enabled);
void nmlModelSetTransparency(float alpha);
void func_001BBB18(void);
void xglLightPackBuildByName(XenoLight *light, void *source, void *block);
void func_001BB96C(XenoLight *light);
void func_001BB9F4(void);
void func_001BBA78(void);
void pxySetFogColor(void *block);
void pxySetFogDist(float r, float g, float b, float a);
void func_001BAA34(void);
void func_001BA924(void);
void func_001BB3CC(float value);
void pxySetModelPosition(Vector4 *position);
void func_001BBAB0(void);
void func_001BA794(void);
void pxySetShadowMode(int mode, float a, float b, float c);
void func_001BADA8(float a, float b);
XenoMatrix *xglPacketReserve(int size);
void pxySetJointMatrices(XenoMatrix *joints, XenoMatrix *global);
void func_001BAE28(XenoMatrix *global);
void tiMotionUpdateVisibility(int *handle, void *visibility);
void func_001BAE58(int frame);
void pxySetTxyBlock(int effect);
void func_001BA9E4(void);
void func_001BA618(float a, float b);
void pxySetClipPlaneWithAngle(void *a, void *b, void *c);
void pxyAddClipPlanePair(void *a, void *b);
void *memset(void *dst, int value, unsigned int size);
void pxySetClipPlaneMasked(void *a, void *b, int count, u32 *mask, float x, float y, float z);
void func_001BA278(void);
void func_001BA9A0(float value);
void func_001BAC70(int flags);
int pxySubmitModel(PxyHeader *pack);
int func_001BBB70(void);

/* actor->drawFlags bits tested by XenoActorDrawMotion; names with a number are set elsewhere and not recovered yet. */
#define XENO_DRAW_COLOR 0x1               /* apply the color block and tint */
#define XENO_DRAW_SKIP_ONCE 0x2           /* cleared on the draw that is skipped */
#define XENO_DRAW_HIDDEN 0x4
#define XENO_DRAW_LIGHT 0x8
#define XENO_DRAW_LIGHT_SOURCE 0x10       /* use sourceLight, refreshed from lightSource */
#define XENO_DRAW_LIGHT_ACTIVE 0x20       /* the light has positive intensity */
#define XENO_DRAW_FOCUS_PENDING 0x40
#define XENO_DRAW_OFFSET_B 0x80
#define XENO_DRAW_FOCUS_LOCKED 0x100
#define XENO_DRAW_FLAG_200 0x200
#define XENO_DRAW_ALPHA 0x10000
#define XENO_DRAW_RENDER_2 0x20000        /* adds render flag 2 */
#define XENO_DRAW_SHADOW_PROJECT 0x40000
#define XENO_DRAW_SHADOW_STENCIL 0x80000
#define XENO_DRAW_FLAG_100000 0x100000
#define XENO_DRAW_FLAG_200000 0x200000
#define XENO_DRAW_FLAG_800000 0x800000
#define XENO_DRAW_FLAG_2000000 0x2000000
#define XENO_DRAW_PARENT_FADE 0x4000000   /* fade by alphaScale */
#define XENO_DRAW_FLAG_8000000 0x8000000
#define XENO_DRAW_DREW 0x10000000         /* set when the pack drew something */
/* Any of these (or a shadow count) enters the shadow/focus block. */
#define XENO_DRAW_POST_MASK 0xA0C01C0
#define XENO_RENDER_FLAG_2 2
/* First three bytes of PxyHeader.magic, "pxy" read little-endian. */
#define PXY_MAGIC 0x797870

/* XenoActorDrawMotion: draws the motion pack (HRC, global matrices and PAM data) of the actor. */
void XenoActorDrawMotion(XenoActor *actor)
{
    PxyHeader *pack;
    XenoMatrix *localMatrix;
    void *visibility;
    PxyHeader *jointPack;
    XenoMatrix *joints;
    XenoMatrix *jointBase;
    PxyHeader *subPack;
    PxyRange *range;
    PxyGeometry *geometry;
    XenoLight *light;
    XenoHrc *hrc;
    XenoMatrix *focus;
    DoublewordAlignedVector4 position;
    int renderFlags;
    u32 flags;
    float scale;
    int joint;
    int part;
    int child;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x5BB);
    }
    /* original keeps this redundant actor != 0 test below; removing it moves code */
    actor->drawFlags &= ~XENO_DRAW_DREW;
    if (actor->drawFlags & (XENO_DRAW_SKIP_ONCE | XENO_DRAW_HIDDEN)) {
        actor->drawFlags &= ~2;
        return;
    }
    XenoActorPushMatrix(actor);
    if (actor->globalMatrices != 0 && (actor->attr & 2)) {
        if (actor->table == 0) {
            assertFailed(D_0068D7A0, xenoActorAssertFile, 0x5CE);
        }
        if (actor->globalMatrices == 0) {
            assertFailed(D_0068D7C0, xenoActorAssertFile, 0x5CF);
        }
        func_00295EE8(actor, actor->globalMatrices, &actor->localMatrix);
        if (actor->attr & 4) {
            XenoActorRefreshItems(actor);
        }
    }
    if (actor != 0 && (pack = actor->pack) != 0 && (*(long long *) pack->magic & 0xFFFFFF) == PXY_MAGIC) {
        localMatrix = &actor->localMatrix;
        if (actor->state != 0) {
            func_001BAA98(actor->state);
            actor->state = 0;
        }
        nmlModelSetDrawMatrix(localMatrix);
        pxySetModelScale(actor->rowC);
        if (actor->colorIndex != 0xFF) {
            func_001BA248(&actor->colorIndex);
        }
        if (actor->parent != 0) {
            func_001BB678(actor->parent->pack);
            if ((actor->parent->drawFlags & XENO_DRAW_PARENT_FADE) && actor->parent->alphaScale < 1.0f) {
                nmlModelSetStencil(1);
                nmlModelSetZwrite(1);
                nmlModelSetTransparency(actor->parent->alpha * actor->parent->alphaScale);
                if (actor->parent->alpha * actor->parent->alphaScale <= 0.0f) {
                    func_001BBB18();
                    return;
                }
            }
        } else {
            if (actor->drawFlags & 8) {
                light = &actor->light;
                if (actor->drawFlags & XENO_DRAW_LIGHT_SOURCE) {
                    light = &actor->sourceLight;
                    xglLightPackBuildByName(light, actor->lightSource, actor->sourceBlock);
                }
                func_001BB96C(light);
                if (light->intensity > 0.0f) {
                    actor->drawFlags |= XENO_DRAW_LIGHT_ACTIVE;
                } else {
                    actor->drawFlags &= ~XENO_DRAW_LIGHT_ACTIVE;
                }
                if (actor->drawFlags & XENO_DRAW_LIGHT_ACTIVE) {
                    func_001BB9F4();
                }
            }
            if (actor->drawFlags & XENO_DRAW_FLAG_100000) {
                func_001BBA78();
            }
            if ((actor->drawFlags & XENO_DRAW_PARENT_FADE) && actor->alphaScale < 1.0f) {
                nmlModelSetStencil(1);
                nmlModelSetZwrite(1);
                nmlModelSetTransparency(actor->alpha * actor->alphaScale);
                if (actor->alpha * actor->alphaScale <= 0.0f) {
                    func_001BBB18();
                    return;
                }
            }
        }
        if (actor->drawFlags & 1) {
            pxySetFogColor(actor->colorBlock);
            pxySetFogDist(actor->color[0], actor->color[1], actor->color[2], actor->color[3]);
        }
        if (actor->drawFlags & XENO_DRAW_ALPHA) {
            range = (PxyRange *) ((char *) pack + pack->rangeOffset);
            nmlModelSetStencil(1);
            if (range->zwrite != 0) {
                nmlModelSetZwrite(1);
            }
            nmlModelSetTransparency(actor->alpha);
            if (actor->alpha <= 0.0f) {
                func_001BBB18();
                return;
            }
        }
        if (actor->drawFlags & XENO_DRAW_FLAG_200000) {
            func_001BAA34();
        }
        visibility = actor->visibility;
        if (actor->drawFlags & XENO_DRAW_FLAG_800000) {
            func_001BA924();
        }
        func_001BB44C(visibility);
        func_001BB3CC(actor->modeValue3);
        position = actor->position;
        position.y += actor->heightOffset;
        pxySetModelPosition(&position);
        if (actor->flags & 0x80) {
            func_001BBAB0();
        }
        if (actor->drawFlags & XENO_DRAW_FLAG_200) {
            func_001BA794();
        }
        switch (actor->drawMode) {
        case 1:
            pxySetShadowMode(1, actor->modeValue, actor->modeValue2, actor->modeValue4);
            if (actor->fogA != 0.0f || actor->fogB != 0.0f) {
                func_001BADA8(actor->fogA, actor->fogB);
            } else {
                scale = 1.0f;
                if (pack->radius >= 3.0f) {
                    scale = 5.0f;
                }
                func_001BADA8(actor->fogC * scale, actor->fogD * scale);
            }
            break;
        case 9:
            pxySetShadowMode(5, actor->modeValue, actor->modeValue2, actor->modeValue4);
            break;
        case 8:
            pxySetShadowMode(4, actor->modeValue, actor->modeValue2, actor->modeValue4);
            func_001BADA8(actor->fogA, actor->fogB);
            break;
        case 2:
            pxySetShadowMode(2, actor->modeValue, actor->modeValue2, actor->modeValue4);
            break;
        case 3:
            pxySetShadowMode(3, actor->modeValue, actor->modeValue2, actor->modeValue4);
            break;
        }
        if (actor->table != 0
            && (actor->motionKeys[0] != 0 || actor->motionKeys[1] != 0 || actor->motionKeys[2] != 0
                || actor->motionKeys[3] != 0)) {
            jointPack = actor->pack;
            if (actor->table->numTransform - 2 < jointPack->jointMatrixCount) {
                assertFailed(D_0068D7E0, xenoActorAssertFile, 0x684);
            }
            if (jointPack->jointMatrixCount != 0) {
                joints = xglPacketReserve(jointPack->jointMatrixCount * 4);
                jointBase = (XenoMatrix *) ((char *) jointPack + jointPack->jointMatrixOffset);
                for (joint = 0; joint < jointPack->jointMatrixCount; joint++) {
                    xglMatrixMul(&joints[joint], &actor->globalMatrices[joint + 2], &jointBase[joint]);
                }
                pxySetJointMatrices(joints, &actor->globalMatrices[2]);
                func_001BAE28(&actor->globalMatrices[2]);
            }
            if (actor->attr & 4) {
                XenoActorRefreshItems(actor);
            }
            tiMotionUpdateVisibility(actor->handle640, visibility);
            flags = actor->drawFlags;
            if (!(flags & XENO_DRAW_FOCUS_LOCKED)) {
                hrc = actor->table;
                if (hrc->focus != 0 && (hrc->flags & 4)) {
                    focus = &actor->matrices[hrc->index10 + hrc->index14 + hrc->index18];
                    actor->drawFlags = flags | XENO_DRAW_FOCUS_PENDING;
                    actor->focusX = focus->m[3][2];
                    actor->focusY = focus->m[3][1];
                } else {
                    actor->focusX = 0.0f;
                    actor->focusY = 0.0f;
                }
            }
        } else if (actor->pamActive != 0) {
            if (actor->pamData == 0) {
                assertFailed(D_0068D828, xenoActorAssertFile, 0x6AE);
            }
            func_001BAE58(actor->pamFrame);
        }
        if (actor->effect != 0) {
            pxySetTxyBlock(actor->effect);
        }
        if ((actor->drawFlags & XENO_DRAW_POST_MASK) || actor->shadowCount != 0) {
            if (actor->drawFlags & XENO_DRAW_FLAG_2000000) {
                func_001BA9E4();
            }
            if (actor->drawFlags & (XENO_DRAW_FOCUS_PENDING | XENO_DRAW_FOCUS_LOCKED)) {
                func_001BA618(actor->focusX, actor->focusY);
                actor->drawFlags &= ~XENO_DRAW_FOCUS_PENDING;
                actor->drawFlags &= ~XENO_DRAW_FOCUS_LOCKED;
            }
            if (actor->drawFlags & XENO_DRAW_OFFSET_B) {
                func_001BA618(actor->modeValue2, actor->modeValue3);
            }
            if (actor->drawFlags & XENO_DRAW_SHADOW_PROJECT) {
                nmlModelSetStencil(1);
                nmlModelSetZwrite(1);
                nmlModelSetTransparency(actor->alpha);
                pxySetClipPlaneWithAngle(actor->shadowA, actor->shadowB, actor->shadowC);
            } else if (actor->drawFlags & XENO_DRAW_SHADOW_STENCIL) {
                pxyAddClipPlanePair(actor->shadowA, actor->shadowB);
                pxyAddClipPlanePair(actor->shadowA2, actor->shadowB2);
                actor->drawFlags &= ~XENO_DRAW_SHADOW_STENCIL;
            }
            if (actor->shadowCount != 0) {
                if (pack->flags & 0x2000) {
                    geometry = (PxyGeometry *) ((char *) pack + pack->geometryOffset);
                    memset(actor->geometryMask, 0, 0x20);
                    for (part = 0; part < pack->geometryCount; part++) {
                        if (geometry->flags & 0x100000) {
                            actor->geometryMask[part >> 5] |= 1 << (part & 0x1F);
                        }
                        geometry++;
                    }
                }
                pxySetClipPlaneMasked(actor->shadowA, actor->shadowB, actor->shadowCount, actor->geometryMask, actor->shadowValue,
                              actor->shadowValue2, actor->shadowValue3);
            }
            if (actor->drawFlags & XENO_DRAW_FLAG_8000000) {
                func_001BA278();
            }
        }
        renderFlags = actor->renderFlags;
        if (actor->drawFlags & XENO_DRAW_RENDER_2) {
            renderFlags |= XENO_RENDER_FLAG_2;
        }
        subPack = actor->pack;
        func_001BA9A0(actor->renderValue);
        func_001BAC70(renderFlags);
        pxySubmitModel(subPack);
        if (func_001BBB70() != 0) {
            actor->drawFlags |= XENO_DRAW_DREW;
        }
        if (subPack->subPackOffset != 0) {
            func_001BB678(subPack);
            nmlModelSetDrawMatrix(localMatrix);
            if (actor->drawFlags & 1) {
                pxySetFogColor(actor->colorBlock);
                pxySetFogDist(actor->color[0], actor->color[1], actor->color[2], actor->color[3]);
            }
            if (actor->drawFlags & 8) {
                func_001BB96C(&actor->light);
            }
            if (actor->effect != 0) {
                pxySetTxyBlock(actor->effect);
            }
            func_001BB44C(actor->block4E0);
            func_001BB47C(actor->block520);
            func_001BA9A0(actor->renderValue);
            func_001BAC70(renderFlags);
            pxySubmitModel((PxyHeader *) ((char *) subPack + subPack->subPackOffset));
        }
    }
    for (child = 0; child < 8; child++) {
        if (actor->children[child] != 0) {
            XenoActorDrawMotion(actor->children[child]);
        }
    }
}


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
    XenoActorPushMatrix(actor);
    xglCurMatrixSet(actor->localMatrix.m[0]);
    xglCurApplyVector(id, &actor->position);
}
