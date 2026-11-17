#include "common.h"
#include "main/TiDrawCollision.h"
#include "main/TiDrawJoint.h"
#include "main/TiMRPackage.h"
#include "main/TiMtnBlend.h"
#include "main/nml_model_set.h"
#include "main/test_128.h"
#include "main/tu026.h"
#include "main/tu255.h"
#include "main/tu256.h"
#include "main/xenoActor.h"
#include "main/xgl_2.h"
#include "main/xgl_studio.h"
#include "quadword.h"
#include "vu0_macro.h"

#include "shared.h"
#include "main/sdv_033.h"

typedef struct XenoMatrix {
    float m[4][4];
} XenoMatrix;

/* Per-joint record of an actor; the translation is accessed through the vector union (the original's float/word access). */
typedef struct XenoJointRecord {
    float row0[4];
    DoublewordVector4 translation;          /* 0x10 */
    float rest[8];
} XenoJointRecord;

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
    unsigned char unmodeled_70[0x80 - 0x70];
    AlignedVector4 sphere;                  /* 0x80: bounding sphere, center xyz and radius in w */
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

/* XenoHrc (the motion hierarchy, actor->table) is declared in main/xenoActor.h. */

/* One joint record of a resource (0x40 bytes). */
typedef struct XenoJoint {
    unsigned char unmodeled_00[0x38];
    int id;                                 /* 0x38 */
    unsigned char unmodeled_3c[4];
} XenoJoint;

/* Resource of an actor (actor->resource). */
typedef struct XenoResource {
    unsigned char unmodeled_00[4];
    float version;                          /* 0x04: below 2.4 the joints come from the hierarchy instead */
    unsigned char unmodeled_08[4];
    u32 matrixCount;                        /* 0x0C: transforms; one 0x40-byte matrix each */
    unsigned char unmodeled_10[0x20 - 0x10];
    u32 extraMatrixCount;                   /* 0x20 */
    u32 jointCount;                         /* 0x24 */
    unsigned char unmodeled_28[0x38 - 0x28];
    int jointOffset;                        /* 0x38: bytes from the resource to its XenoJoint array */
} XenoResource;

/* Range table of one slot: entryCount offsets (bytes from the table) to entries holding two bounds. */
typedef struct XenoSlotTable {
    unsigned char unmodeled_00[8];
    u32 entryCount;                         /* 0x08 */
    int entryOffset[1];                     /* 0x0C: entryCount offsets */
} XenoSlotTable;

typedef struct XenoSlotRange {
    unsigned char unmodeled_00[8];
    float lower;                            /* 0x08 */
    float upper;                            /* 0x0C */
} XenoSlotRange;

typedef struct XenoLight {
    unsigned char unmodeled_000[0x12C];
    float intensity;                        /* 0x12C */
    unsigned char unmodeled_130[0x180 - 0x130];
} XenoLight;

typedef struct XenoActorEntry {
    unsigned char unmodeled_00[0x08];
    f32 value;                      /* +0x08 */
    f32 upper;                      /* +0x0C */
} XenoActorEntry;

typedef struct XenoActor {
    unsigned short attr;                    /* 0x000: bit 2 = has global matrices */
    unsigned char unmodeled_002[0x10 - 0x2];
    DoublewordAlignedVector4 rowA;          /* 0x010 */
    float prevRowA[4];                      /* 0x020: copy of the row at 0x10 */
    float angle[4];                         /* 0x030: copy of rowB, its three angles wrapped into [0, 2*pi) */
    unsigned char unmodeled_040[0x50 - 0x40];
    float rowB[3];                          /* 0x050 */
    unsigned char unmodeled_05c[0x70 - 0x5C];
    float rowD[4];                          /* 0x070 */
    DoublewordAlignedVector4 position;      /* 0x080 */
    float rowC[4];                          /* 0x090 */
    unsigned char unmodeled_0a0[0xB0 - 0xA0];
    float modeValueB0;                      /* 0x0B0 */
    unsigned char unmodeled_0b4[0xBC - 0xB4];
    u32 drawFlags;                          /* 0x0BC */
    float alpha;                            /* 0x0C0 */
    int drawMode;                           /* 0x0C4 */
    unsigned char unmodeled_0c8[4];
    float modeValue;                        /* 0x0CC */
    int renderFlags;                        /* 0x0D0 */
    float renderValue;                      /* 0x0D4 */
    float renderValue2;                     /* 0x0D8 */
    unsigned char lightSource[0x100 - 0xDC]; /* 0x0DC */
    XenoMatrix localMatrix;                 /* 0x100 */
    float focusX;                           /* 0x140 */
    float focusY;                           /* 0x144 */
    float modeValue2;                       /* 0x148 */
    float modeValue3;                       /* 0x14C */
    float field150;                         /* 0x150 */
    float field154;                         /* 0x154 */
    float shadowValue;                      /* 0x158 */
    int shadowCount;                        /* 0x15C */
    int moveState;                          /* 0x160: clip-plane shape selector of XenoActorUpdateMoveState */
    float shadowValue2;                     /* 0x164 */
    float shadowValue3;                     /* 0x168 */
    float heightOffset;                     /* 0x16C */
    float fogA;                             /* 0x170 */
    float fogB;                             /* 0x174 */
    float fogC;                             /* 0x178 */
    float fogD;                             /* 0x17C */
    float modeValue4;                       /* 0x180 */
    float field184;                         /* 0x184 */
    float alphaScale;                       /* 0x188 */
    float fadeRadius;                       /* 0x18C: fade distance override (0 = use the pack radius) */
    XenoLight light;                        /* 0x190 */
    XenoLight sourceLight;                  /* 0x310 */
    float color[4];                         /* 0x490 */
    unsigned char colorBlock[0x10];         /* 0x4A0 */
    unsigned char sourceBlock[0x10];        /* 0x4B0 */
    u32 visibility[8];                      /* 0x4C0: one bit per geometry of the pack */
    u32 subVisibility[8];                   /* 0x4E0: one bit per geometry of the sub pack */
    u32 geometryMask[8];                    /* 0x500: one bit per shadow-casting geometry */
    unsigned char block520[0x20];           /* 0x520 */
    Vector4 shadowA;                        /* 0x540 */
    unsigned char shadowA2[0x10];           /* 0x550 */
    Vector4 shadowB;                        /* 0x560 */
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
    int slot;                               /* 0x5D8: mode slot of the current mode */
    int requested_mode;                     /* 0x5DC: XenoActorRequestMode stores its argument here */
    float cachedValue;                      /* 0x5E0 */
    int cachedSelector;                     /* 0x5E4 */
    unsigned char unmodeled_5e8[0x5EC - 0x5E8];
    PxyHeader *pack;                        /* 0x5EC */
    int effect;                             /* 0x5F0 */
    XenoResource *resource;                 /* 0x5F4 */
    int entryB;                             /* 0x5F8 */
    struct XenoSlotTable *slotTable[6];     /* 0x5FC: one range table per slot (xenoActorSlotRangeValid, XenoActorGetSlotRange) */
    int pamActive;                          /* 0x614 */
    int entryTableC;                        /* 0x618 */
    unsigned char unmodeled_61c[4];
    int link;                               /* 0x620 */
    unsigned char unmodeled_624[4];
    void *matrixBuf628;                     /* 0x628: heap-5 buffers, one 0x40-byte matrix per hierarchy transform */
    void *matrixBuf62C;                     /* 0x62C */
    void *matrixBuf630;                     /* 0x630 */
    int pamFrame;                           /* 0x634 */
    void *matrixBuf638;                     /* 0x638 */
    unsigned char unmodeled_63c[4];
    int handle640[4];                       /* 0x640: its address is passed to callees (the 0x60-byte channel of XenoActorSetSelector); element 1 is stored by func_0028B2F0 */
    int flags650;                           /* 0x650 */
    float motionSpeed;                      /* 0x654: its sign picks which bound XenoActorSetPlayRange restarts from */
    float motionTime;                       /* 0x658: channel value read back after the motion package advances */
    unsigned char unmodeled_65c[0x660 - 0x65C];
    int source_value;                       /* 0x660 */
    unsigned char unmodeled_664[0x670 - 0x664];
    Vector4 focusTarget;                    /* 0x670: world point the actor looks at (XenoActorSetFocusTarget) */
    Vector4 focusPrevious;                  /* 0x680: copy of focusCurrent made by XenoActorSetFocusTarget */
    Vector4 focusCurrent;                   /* 0x690 */
    int flags6A0;                           /* 0x6A0: the channel flags; XenoActorSetSelector gates its setup on bit 0x2 */
    unsigned char unmodeled_6A4[0x6A8 - 0x6A4];
    float cachedMotionTime;                 /* 0x6A8: second channel value read back after its package advances */
    unsigned char unmodeled_6AC[0x700 - 0x6AC];
    struct XenoActorEntry *motionKeys[4];   /* 0x700: one per mode slot; XenoActorSetSelector reads slot 2 */
    XenoHrc *table;                         /* 0x710 */
    void *pamData;                          /* 0x714 */
    unsigned char unmodeled_718[4];
    XenoMatrix *matrices;                   /* 0x71C */
    XenoMatrix *globalMatrices;             /* 0x720 */
    unsigned char unmodeled_724[0x72C - 0x724];
    XenoJointRecord *jointRecords;          /* 0x72C: one 64-byte record per hierarchy joint (XenoHrc.index20 of them) */
    XenoMatrix *channelMatrix;              /* 0x730: set to localMatrix when the channel is updated */
    unsigned char unmodeled_734[0x740 - 0x734];
    unsigned char probe[0x79C - 0x740];     /* 0x740: map probe request, initialized by mapProbeInit */
    int state;                              /* 0x79C: tu248 writes it; XenoActorDrawMotion reads and clears it */
    short field7A0;                         /* 0x7A0 */
    short field7A2;                         /* 0x7A2 */
    short activeItem;                       /* 0x7A4: item index XenoActorRefreshItems compares against its loop counter */
    unsigned char unmodeled_7a6[0x7F0 - 0x7A6]; /* pool record size is 0x7F0 */
} XenoActor;

/* Bits of XenoActor.flags. */
#define XENO_FLAG_FREEZE_A 0x1           /* XenoActorUpdateTree does not advance motion A */
#define XENO_FLAG_UNWRAP_A 0x2           /* motion A advances through the unwrapped variant */
#define XENO_FLAG_FREEZE_B 0x4           /* XenoActorUpdateTree does not advance motion B */
#define XENO_FLAG_UNWRAP_B 0x8           /* motion B advances through the unwrapped variant */
#define XENO_FLAG_BLEND_SWAPPED 0x10     /* the channel blend was swapped in; cleared when the mode is re-applied */
#define XENO_FLAG_RANGE_SET 0x40         /* a play range was set (XenoActorSetPlayRange) */
#define XENO_FLAG_CHANNEL_BIT20 0x80     /* either channel flag word has bit 0x20 set */
#define XENO_FLAG_DRAW_POINTS 0x100      /* XenoActorSetFocusTarget draws the focus joint points */
#define XENO_FLAG_RESET_CHANNEL 0x200    /* XenoActorUpdateTree restarts the channel */
#define XENO_FLAG_SHIFT_JOINTS 0x400     /* set by func_0028AF28: XenoActorUpdateTree shifts the joint rows */
#define XENO_FLAG_SKIP_DELTA 0x800       /* set by XenoActorWrapRotation: next update only snapshots the joint rows */

/* colorIndex value meaning "no color"; XenoActorInit fills the 0x10-byte block at colorIndex with it. */
#define XENO_COLOR_INDEX_NONE 0xFF

extern int XenoActorPoolCount;
extern XenoActor *XenoActorPool;

/* Size in bytes of one pool record. */
#define XENO_ACTOR_POOL_STRIDE 0x7F0

/* XenoActorPoolGet: pool record at index (records are 0x7F0 bytes apart), or 0 outside XenoActorPoolCount. */
XenoActor *XenoActorPoolGet(int index)
{
    if (index < 0 || index >= XenoActorPoolCount) {
        return 0;
    }
    return (XenoActor *)((unsigned char *)XenoActorPool + index * XENO_ACTOR_POOL_STRIDE);
}

extern u8 D_008A2CB0[];
extern int *XenoActorActiveTable;

/* XenoActorPoolInit: sets the pool base and count, builds the active table after the pool and inits every record. */
unsigned char *XenoActorPoolInit(void *data, int size) {
    unsigned char *end;
    int i;
    XenoActor *record;
    int *slot;

    XenoActorPoolCount = size;
    end = data;
    XenoActorPool = (XenoActor *)end;
    end += size * XENO_ACTOR_POOL_STRIDE;
    XenoActorActiveTable = (int *)end;
    end += size * 4;
    record = data;
    for (i = 0; i < XenoActorPoolCount; i++) {
        XenoActorInit(record++);
    }
    if (XenoActorPoolCount > 0) { slot = XenoActorActiveTable; i = XenoActorPoolCount; do { *slot++ = 0; } while (--i); }
    XenoActorInitHeap((int)D_008A2CB0, 0x38000);
    XenoActorClearHoldFlag();
    return end;
}

extern u8 D_0088AEF0[];

/* XenoActorPoolInitDefault: initializes the pool from the built-in buffer D_0088AEF0 (0x30 records). */
void XenoActorPoolInitDefault(void) {
    XenoActorPoolInit(D_0088AEF0, 0x30);
}

extern unsigned char xenoActorAssertFile[];
extern unsigned char xenoActorAssertExpr[];
struct RgHeap *InstanceOfRgHeapById(int);
void tiMRStateSetValue(void *, float);
int tiMRPackageGetNodeMatrix(int);
int tiMRPackageGetNodePoint(int);
void xglCurMatrixSet(f32 *);
void xacEvalTransform(float *, int *, float);
extern int XenoActorHoldFlag;
void xglCurSetTranslate(const Vector4 *vector);
void tiDrawJointDrawPoints(f32 matrix[4][4], float scale);
void xglMatrixMul(void *, void *, void *);

/* XenoActorInitHeap: initializes RG heap 5, the heap the actor matrices are allocated from, over the given memory block. */
void XenoActorInitHeap(int memory, int size) {
    InitRgHeap(InstanceOfRgHeapById(5), (void *)memory, size); /* memory is an int address in the original's prototype */
}

void *memset(void *dst, int value, unsigned int size);
void tiMRPackageSetEntryA(void *child, int flags);

/* Not in xenoActor.h: other files declare these two with different types. */
void XenoActorSetMode(XenoActor *actor, int mode);

/* XenoActorInit: resets one record to its default state (slotIndex and mode -1, default channel values). */
void XenoActorInit(XenoActor *actor) {
    int *handle = actor->handle640;

    if (actor == 0) {
        return;
    }
    memset(actor, 0, sizeof(XenoActor));
    actor->field7A0 = -1;
    actor->activeItem = -1;
    actor->field7A2 = -1;
    actor->modeValue = 0.2f;
    actor->drawMode = 0;
    actor->modeValueB0 = 0.3f;
    actor->requested_mode = -1;
    actor->slotIndex = -1;
    actor->fogC = 0.5f;
    actor->fogD = -1.0f;
    actor->rowA.w = 1.0f;
    actor->rowD[3] = 1.0f;
    actor->alpha = 1.0f;
    actor->rowC[3] = 1.0f;
    actor->rowC[2] = 1.0f;
    actor->rowC[1] = 1.0f;
    actor->rowC[0] = 1.0f;
    actor->renderValue2 = 1.0f;
    actor->field154 = 1.0f;
    actor->shadowValue2 = 1.0f;
    actor->shadowValue3 = 1.0f;
    actor->modeValue4 = 1.0f;
    actor->modeValue3 = 0.0f;
    actor->shadowValue = 0.0f;
    actor->field150 = 0.0f;
    actor->shadowCount = 0;
    actor->fogA = 0.0f;
    actor->fogB = 0.0f;
    actor->modeValue2 = 0.0f;
    actor->field184 = 0.0f;
    actor->fadeRadius = 0.0f;
    memset(&actor->colorIndex, XENO_COLOR_INDEX_NONE, 0x10);
    tiMRPackageInit(handle);
    tiMRPackageSetEntryA(handle, 0);
    memset(actor->geometryMask, 0xFF, sizeof(actor->geometryMask));
    xglLightSetDefault(&actor->light);
    mapProbeInit(actor->probe);
}

/* XenoActorAlloc: takes the first free record from the pool, initializes it and adds it to the active table. */
XenoActor *XenoActorAlloc(void) {
    unsigned int i;
    int slot;
    XenoActor *pool;
    XenoActor *actor = 0;

    for (i = 0; i < XenoActorPoolCount; i++) {
        if (XenoActorPool[i].attr == 0) {
            XenoActorInit(&XenoActorPool[i]);
            pool = XenoActorPool;
            /* Never taken (actor is still 0 here); the original keeps this check, which
             * splits the pool reload from the attr store (gcse then shares the reload).
             * Real conditions tried in its place all change the code: "pool[i].attr != 0" (69 rows),
             * "pool == 0" (62 rows), "i >= XenoActorPoolCount" (92 rows); each is folded or reordered differently. */
            if (actor != 0) {
                return actor;
            }
            pool[i].attr = 1;
            for (slot = 0; slot < XenoActorPoolCount; slot++) {
                if (XenoActorActiveTable[slot] == 0) {
                    XenoActorActiveTable[slot] = (int)&pool[i];
                    break;
                }
            }
            XenoActorPool[i].flags |= XENO_FLAG_RESET_CHANNEL;
            return &XenoActorPool[i];
        }
    }
    return 0;
}

extern unsigned char D_0068D700[];
extern unsigned char D_0068D718[];
extern unsigned char D_0068D730[];
extern unsigned char D_0068D748[];
extern unsigned char D_0068D760[];
void *RgHeapAllocById(u32 heapId, u32 size, const char *pSourceFile, int line);
void RgHeapFree(void *pPtr, const char *pSourceFile, int line);
void tiMRPackageSetLocalMat(void *package, void *matrices);
void tiMRPackageSetGlobalMat(void *package, void *matrices);
void func_00291F80(void *package, void *matrices);
void tiMRPackageSetPamFrame(void *package, void *matrices);
void tiMRPackageSetHrcData(void *package, XenoResource *resource);
void tiMRPackageSetTableA(void *package, void *table);
void tiMRPackageSetEntryB(void *package, int entry);
void tiMRPackageSetTableB(void *package, void *table);
void func_002922F0(void *package, int value);
void tiMRPackageFillNodes(void *package, void *nodes);
void tiMRPackageSetClampRange(void *package, int clampRange);

/* XenoActorAllocMatrices: frees and reallocates the matrix buffers of a record from its resource, then rebuilds the motion channel at 0x640. */
int XenoActorAllocMatrices(XenoActor *actor) {
    XenoResource *resource;
    void *package;
    int entryA, entryB, rangeC;
    void *tableA, *tableB;
    int clampRange;
    PxyHeader *pack;

    if (actor->matrixBuf628 != 0) {
        RgHeapFree(actor->matrixBuf628, xenoActorAssertFile, 0xB5);
        actor->matrixBuf628 = 0;
    }
    if (actor->matrixBuf62C != 0) {
        RgHeapFree(actor->matrixBuf62C, xenoActorAssertFile, 0xB6);
        actor->matrixBuf62C = 0;
    }
    if (actor->matrixBuf630 != 0) {
        RgHeapFree(actor->matrixBuf630, xenoActorAssertFile, 0xB7);
        actor->matrixBuf630 = 0;
    }
    if (actor->matrixBuf638 != 0) {
        RgHeapFree(actor->matrixBuf638, xenoActorAssertFile, 0xB8);
        actor->matrixBuf638 = 0;
    }
    resource = actor->resource;
    if (resource != 0) {
        entryA = actor->entryB;
        tableA = actor->slotTable[0];
        rangeC = actor->entryTableC;
        entryB = (int)actor->slotTable[4];
        tableB = actor->slotTable[5];
        actor->matrixBuf628 = RgHeapAllocById(5, resource->matrixCount << 6, xenoActorAssertFile, 0xC7);
        actor->matrixBuf62C = RgHeapAllocById(5, resource->matrixCount << 6, xenoActorAssertFile, 0xC8);
        actor->matrixBuf630 = RgHeapAllocById(5, resource->matrixCount << 6, xenoActorAssertFile, 0xC9);
        if (resource->extraMatrixCount != 0) {
            actor->matrixBuf638 = RgHeapAllocById(5, resource->extraMatrixCount << 6, xenoActorAssertFile, 0xCD);
            if (actor->matrixBuf638 == 0) {
                assertFailed(D_0068D700, xenoActorAssertFile, 0xCE);
            }
        }
        if (actor->matrixBuf628 == 0) {
            assertFailed(D_0068D718, xenoActorAssertFile, 0xD3);
        }
        if (actor->matrixBuf62C == 0) {
            assertFailed(D_0068D730, xenoActorAssertFile, 0xD4);
        }
        if (actor->matrixBuf630 == 0) {
            assertFailed(D_0068D748, xenoActorAssertFile, 0xD5);
        }
        package = actor->handle640;
        tiMRPackageInit(package);
        tiMRPackageSetLocalMat(package, actor->matrixBuf628);
        tiMRPackageSetGlobalMat(package, actor->matrixBuf62C);
        func_00291F80(package, actor->matrixBuf630);
        tiMRPackageSetHrcData(package, resource);
        tiMRPackageSetEntryA(package, entryA);
        tiMRPackageSetTableA(package, tableA);
        tiMRPackageSetEntryB(package, entryB);
        tiMRPackageSetTableB(package, tableB);
        func_002922F0(package, rangeC);
        tiMRPackageFillNodes(package, actor->matrixBuf638);
        XenoActorSetMode(actor, 0);
        XenoActorSetSelector(actor, 0);
    }
    clampRange = actor->pamActive;
    if (clampRange != 0) {
        pack = actor->pack;
        if (pack != 0) {
            if (actor->pamFrame != 0) {
                RgHeapFree((void *)actor->pamFrame, xenoActorAssertFile, 0xED);
            }
            actor->pamFrame = (int)RgHeapAllocById(5, pack->geometryCount << 6, xenoActorAssertFile, 0xEE);
            if (actor->pamFrame == 0) {
                assertFailed(D_0068D760, xenoActorAssertFile, 0xF0);
            }
            package = actor->handle640;
            tiMRPackageInit(package);
            tiMRPackageSetPamFrame(package, (void *)actor->pamFrame);
            tiMRPackageSetClampRange(package, clampRange);
        }
    }
    return 1;
}

/* XenoActorAttachChild: links a live child under a parent's child slots (0x5AC) and sets the child parent (0x5A4). */
int XenoActorAttachChild(XenoActor *parent, XenoActor *child) {
    int i;

    if (parent != 0 && child != 0 && parent->attr != 0 && child->attr != 0 && child->parent == 0) {
        for (i = 0; i < 8; i++) {
            if (parent->children[i] == 0) {
                parent->children[i] = child;
                child->parent = parent;
                return 1;
            }
        }
    }
    return 0;
}

int XenoActorAppendChildOnJoint(XenoActor *parent, XenoActor *child, int joint) {
    int i;
    if (parent != 0 && child != 0 && parent->attr != 0 && child->attr != 0 && child->parent == 0) {
        for (i = 0; i < 8; i++) {
            if (parent->children[i] == 0) {
                if (joint >= 0) {
                    child->overrideMatrix = parent->globalMatrices + joint;
                } else {
                    child->overrideMatrix = 0;
                }
                parent->children[i] = child;
                child->parent = parent;
                return 1;
            }
        }
    }
    return 0;
}

/* XenoActorAttachChildToJoint: finds the joint with the given id in the resource joint table and attaches the child there. */
int XenoActorAttachChildToJoint(XenoActor *actor, XenoActor *child, int jointId) {
    XenoResource *resource = actor->resource;
    XenoJoint *joints;
    int found;
    u32 i;
    if (resource == 0) {
        return 0;
    }
    if (resource->version < 2.4f) {
        if (jointId >= actor->table->jointCount) {
            return 0;
        }
        return XenoActorAppendChildOnJoint(actor, child, actor->table->index10 + actor->table->index14 + actor->table->focus + actor->table->index20 + jointId);
    }
    joints = (XenoJoint *)((unsigned char *)resource + resource->jointOffset);
    found = -1;
    for (i = 0; i < resource->jointCount; i++) {
        if (joints[i].id == jointId) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return 0;
    }
    return XenoActorAppendChildOnJoint(actor, child, actor->table->index10 + actor->table->index14 + actor->table->focus + actor->table->index20 + found);
}

/* XenoActorAttachAtSlot: stores the slot index, attaches the child to parent and returns 1; 0 on bad arguments. */
int XenoActorAttachAtSlot(int parent, struct XenoActor *actor, int slotIndex) {
    if (parent != 0 && actor != 0 && slotIndex >= 0) {
        actor->slotIndex = slotIndex;
        XenoActorAttachChild((XenoActor *)parent, actor);
        return 1;
    }
    return 0;
}

int XenoActorReleaseChild(XenoActor *parent, XenoActor *child) {
    int i;

    if (parent != 0 && child != 0 && parent->attr != 0 && child->attr != 0 && child->parent != 0) {
        for (i = 0; i < 8; i++) {
            if (parent->children[i] == child) {
                parent->children[i] = 0;
                child->slotIndex = -1;
                child->parent = 0;
                child->overrideMatrix = 0;
                return 1;
            }
        }
    }
    return 0;
}

/* XenoActorSetFlagAll: ORs a flag mask into the drawFlags of every live record.
   The pool count is read again after each change (the reload is in the original code). */
void XenoActorSetFlagAll(int flag) {
    unsigned int i;
    unsigned int count = XenoActorPoolCount;

    for (i = 0; i < count; i++) {
        if (XenoActorPool[i].attr != 0) {
            XenoActorPool[i].drawFlags |= flag;
            count = XenoActorPoolCount;
        }
    }
}

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

void XenoActorClearChannel(XenoActor *actor) {
    tiMRPackageSetTableA(actor->handle640, 0);
    tiMRPackageSetEntryA(actor->handle640, 0);
}

f32 tiMRStateSetPosition(void *channelFlags, f32 value);
f32 tiMtnBlendSwapGlobal(void *channel, void *channelFlags);
void tiMRPackageSetTableAndEntryA(void *channel, void *table, int mode);
void tiMRPackageUpdate(void *channel, void *matrix);
void tiPartsAnimSamplePose(int frame, void *channel, int count);

/* XenoActorSetMode: sets the current mode (slot 0), clears the request and restarts channel 0x640 at the start (or end) of the mode's range. */
void XenoActorSetMode(XenoActor *actor, int mode) {
    XenoActorEntry *entry;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x1F6);
    }
    actor->requested_mode = -1;
    actor->mode = mode;
    actor->slot = 0;
    tiMRPackageSetTableAndEntryA(actor->handle640, actor->slotTable[0], mode);
    actor->flags &= ~XENO_FLAG_RANGE_SET;
    tiWordClearBits(&actor->flags650, 8);
    tiWordClearBits(&actor->flags6A0, 8);
    entry = actor->motionKeys[0];
    if (entry != 0) {
        if (actor->motionSpeed < 0.0f) {
            actor->transformScale = entry->upper;
        } else {
            actor->transformScale = entry->value;
        }
        tiMRStateSetPosition(&actor->flags650, actor->transformScale);
        if (!(actor->flags & XENO_FLAG_BLEND_SWAPPED) && (actor->flags650 & 2)) {
            tiMtnBlendSwapLocal((struct TiMtnBlendContext *)actor->handle640, &actor->flags650);
            tiMtnBlendSwapGlobal(actor->handle640, &actor->flags650);
            actor->flags |= XENO_FLAG_BLEND_SWAPPED;
        }
    }
}

void XenoActorCheckMode(XenoActor *actor, int expected_mode) {
    if (actor->mode != expected_mode) {
        XenoActorSetMode(actor, expected_mode);
    }
}

/* XenoActorSetPlayRange: sets the playback range and clamps the playback value into it. */
void tiMRStateSetRange(void *state, int rangeMin, int rangeMax);
/* XenoActorSetPlayRange: marks the range as set (flag 0x40); when the play position lies outside [start, end] restarts it from start, or from end when the motion runs backwards; then forwards the range to the state. */
void XenoActorSetPlayRange(XenoActor *actor, int start, int end) {
    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x22F);
    }
    actor->flags |= XENO_FLAG_RANGE_SET;
    if (actor->transformScale < (float)start || (float)end < actor->transformScale) {
        if (actor->motionSpeed < 0.0f) {
            actor->transformScale = (float)end;
        } else {
            actor->transformScale = (float)start;
        }
    }
    tiMRStateSetRange(&actor->flags650, start, end);
}

void XenoActorConvertSourceRecord(void *child);

void XenoActorApplySourceValue(XenoActor *actor) {
    actor->transformScale = (f32) actor->source_value;
    XenoActorConvertSourceRecord(&actor->flags650);
}

/* XenoActorSetModeInSlot: like XenoActorSetMode, for one of the four mode slots (asserts slot < 4). */
void XenoActorSetModeInSlot(XenoActor *actor, u32 slot, int mode) {
    XenoActorEntry *entry;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x24D);
    }
    if (slot < 4) {
        actor->requested_mode = -1;
        actor->slot = slot;
        actor->mode = mode;
        tiMRPackageSetTableAndEntryA(actor->handle640, actor->slotTable[slot], mode);
        actor->flags &= ~XENO_FLAG_RANGE_SET;
        tiWordClearBits(&actor->flags650, 8);
        tiWordClearBits(&actor->flags6A0, 8);
        entry = actor->motionKeys[0];
        if (entry != 0) {
            if (actor->motionSpeed < 0.0f) {
                actor->transformScale = entry->upper;
            } else {
                actor->transformScale = entry->value;
            }
            tiMRStateSetPosition(&actor->flags650, actor->transformScale);
            if (!(actor->flags & XENO_FLAG_BLEND_SWAPPED) && (actor->flags650 & 2)) {
                tiMtnBlendSwapLocal((struct TiMtnBlendContext *)actor->handle640, &actor->flags650);
                tiMtnBlendSwapGlobal(actor->handle640, &actor->flags650);
                actor->flags |= XENO_FLAG_BLEND_SWAPPED;
            }
        }
    }
}

/*
 * Partial view of the xenoActor record: only the offsets this TU's claimed
 * functions touch are named (XenoActorSetSelector at +0x5CC..+0x7A4 and
 * XenoActorRefreshItems at +0x710 and +0x7A4). Gaps are unmodeled spans.
 */

typedef struct XenoActorItem {
    unsigned char unmodeled_00[0x04];
    f32 weight;                     /* +0x04 */
    unsigned char unmodeled_08[0x20 - 0x08];
    unsigned char payload[0x10];    /* +0x20: passed by address */
} XenoActorItem;

f32 tiMRPackageSelectEntryB(void *channel, int selector);

/* XenoActorSetSelector: stores the selector, reads the channel value and copies the entry value into cachedValue. */
f32 XenoActorSetSelector(XenoActor *actor, int selector)
{
    XenoActorEntry *entry;
    f32 result;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x27D);
    }
    actor->cachedSelector = selector;
    result = tiMRPackageSelectEntryB(actor->handle640, selector);
    entry = actor->motionKeys[2];
    if (entry != 0) {
        f32 entryValue = entry->value;

        actor->cachedValue = entryValue;
        result = tiMRStateSetPosition(&actor->flags6A0, entryValue);
        if (!(actor->flags & XENO_FLAG_BLEND_SWAPPED) && (actor->flags6A0 & 2)) {
            tiMtnBlendSwapLocal((struct TiMtnBlendContext *)actor->handle640, &actor->flags6A0);
            result = tiMtnBlendSwapGlobal(actor->handle640, &actor->flags6A0);
            actor->flags |= XENO_FLAG_BLEND_SWAPPED;
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
        actor->rowA.x = block[0];
        actor->rowA.y = block[1];
        actor->rowA.z = block[2];
        actor->rowB[0] = block[3];
        actor->rowB[1] = block[4];
        actor->rowB[2] = block[5];
        actor->rowC[0] = block[6];
        actor->rowC[1] = block[7];
        actor->rowC[2] = block[8];
    }
}

/* XenoActorApplyRequestedMode: applies a pending mode request, then updates channel 0x640. */
void XenoActorApplyRequestedMode(XenoActor *actor) {
    PxyHeader *pack;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x2CF);
    }
    if (actor->requested_mode >= 0 && XenoActorIsDone(actor)) {
        XenoActorEnableChannel(actor);
        XenoActorSetMode(actor, actor->requested_mode);
        actor->requested_mode = -1;
    }
    if (actor->table != 0) {
        actor->flags &= ~XENO_FLAG_CHANNEL_BIT20;
        actor->channelMatrix = &actor->localMatrix;
        tiMRPackageUpdate(actor->handle640, &actor->localMatrix);
        if ((actor->flags650 & 0x20) || (actor->flags6A0 & 0x20)) {
            actor->flags |= XENO_FLAG_CHANNEL_BIT20;
        }
    }
    if (actor->pamData != 0 && (pack = actor->pack) != 0) {
        if (actor->pamFrame == 0) {
            assertFailed(D_0068D760, xenoActorAssertFile, 0x2F4);
        }
        tiPartsAnimSamplePose(actor->pamFrame, actor->handle640, pack->geometryCount);
    }
    actor->flags &= ~XENO_FLAG_BLEND_SWAPPED;
    tiWordClearBits(&actor->flags650, 0x10);
}

/* xenoActorSlotRangeValid: 1 when the lower bound of entry idx of the slot's range table does not exceed its upper bound. */
int xenoActorSlotRangeValid(XenoActor *actor, int slot, unsigned int idx) {
    XenoSlotTable *table = actor->slotTable[slot];
    XenoSlotRange *range;

    if (table == 0) {
        return 0;
    }
    if (idx >= table->entryCount) {
        return 0;
    }
    /* offset read as *(base + idx): the indexed form swaps the addu operands */
    range = (XenoSlotRange *)((unsigned char *)table + *(table->entryOffset + idx));
    if (range->lower <= range->upper) {
        return 1;
    }
    return 0;
}

/* XenoActorSetHoldFlag: sets the global update hold XenoActorHoldFlag; XenoActorDoit skips every tree update while it is set. */
void XenoActorSetHoldFlag(void) {
    XenoActorHoldFlag = 1;
}

/* XenoActorClearHoldFlag: clears the global update hold XenoActorHoldFlag; XenoActorPoolInit clears it when the pool is built. */
void XenoActorClearHoldFlag(void) {
    XenoActorHoldFlag = 0;
}

extern unsigned char D_0068D770[];
/* XenoActorRelease: frees the matrix buffers, removes the record from the active table and its parent, detaches its children, then resets it. */
void XenoActorRelease(XenoActor *actor) {
    int i, j;
    int found;
    XenoActor *parent;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x331);
    }
    if (actor->matrixBuf628 != 0) {
        RgHeapFree(actor->matrixBuf628, xenoActorAssertFile, 0x333);
        actor->matrixBuf628 = 0;
    }
    if (actor->matrixBuf62C != 0) {
        RgHeapFree(actor->matrixBuf62C, xenoActorAssertFile, 0x334);
        actor->matrixBuf62C = 0;
    }
    if (actor->matrixBuf630 != 0) {
        RgHeapFree(actor->matrixBuf630, xenoActorAssertFile, 0x335);
        actor->matrixBuf630 = 0;
    }
    if (actor->pamFrame != 0) {
        RgHeapFree((void *)actor->pamFrame, xenoActorAssertFile, 0x336);
        actor->pamFrame = 0;
    }
    if (actor->matrixBuf638 != 0) {
        RgHeapFree(actor->matrixBuf638, xenoActorAssertFile, 0x337);
        actor->matrixBuf638 = 0;
    }
    found = 0;
    for (i = 0; i < XenoActorPoolCount; i++) {
        if ((XenoActor *)XenoActorActiveTable[i] == actor) {
            XenoActorActiveTable[i] = 0;
            found = 1;
            break;
        }
    }
    if (!found) {
        assertFailed(D_0068D770, xenoActorAssertFile, 0x353);
    }
    parent = actor->parent;
    if (parent != 0) {
        for (j = 0; j < 8; j++) {
            if (parent->children[j] == actor) {
                parent->children[j] = 0;
                break;
            }
        }
    }
    for (j = 0; j < 8; j++) {
        if (actor->children[j] != 0) {
            actor->children[j]->parent = 0;
        }
    }
    XenoActorInit(actor);
}

void xglMatrixStackRotY(float);
void xglMatrixStackMulVector(Vector4 *dst, Vector4 *src);

/* XenoActorUpdateMoveState (XenoActorUpdateMoveState): advances the clip-plane sweep of an actor (phase at +0x150, direction at +0x154) and
   builds its clip plane pair (shadowA/shadowB) for the current shape state (+0x160). */
void XenoActorUpdateMoveState(XenoActor *actor) {
    float t;
    float scale;
    float speed;

    if (actor->shadowCount == 0) {
        return;
    }
    speed = 0.0f;
    scale = 1.0f;
    actor->shadowValue2 = scale;
    if (actor->pack != 0 && actor->pack->magic[0] == 'p' && actor->pack->magic[1] == 'x' && actor->pack->magic[2] == 'y') {
        scale = actor->pack->sphere.w;
        if (speed < scale) {
            actor->shadowValue2 = scale;
        }
    }
    speed = actor->field154;
    actor->field150 += __builtin_fabsf(speed) * 0.033333335f;
    if (speed < 0.0f) {
        if (actor->field150 >= 1.0f) {
            actor->field150 = 0.0f;
            actor->shadowCount = 0;
        }
    }
    switch (actor->moveState) {
    case 0:
        if (speed < 0.0f) {
            speed = actor->field150 * scale - scale * 0.5f;
        } else {
            speed = (1.0f - actor->field150) * scale - scale * 0.5f;
        }
        actor->shadowB.y = actor->rowB[1];
        actor->shadowA.z = speed;
        actor->shadowA.w = 1.0f;
        actor->shadowB.x = 0.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.y = 0.0f;
        xglMatrixStackUnit();
        xglMatrixStackRotY(actor->rowB[1]);
        xglMatrixStackMulVector(&actor->shadowA, &actor->shadowA);
        break;
    case 1:
        if (speed < 0.0f) {
            speed = actor->field150 * scale - scale * 0.5f;
        } else {
            speed = (1.0f - actor->field150) * scale - scale * 0.5f;
        }
        actor->shadowB.y = actor->rowB[1] + 3.1415920f;
        actor->shadowA.z = speed;
        actor->shadowA.w = 1.0f;
        actor->shadowB.x = 0.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.y = 0.0f;
        xglMatrixStackUnit();
        xglMatrixStackRotY(actor->rowB[1] + 3.1415920f);
        xglMatrixStackMulVector(&actor->shadowA, &actor->shadowA);
        break;
    case 2:
        t = (speed < 0.0f ? actor->field150 : 1.0f - actor->field150) * scale * 2.0f;
        actor->shadowB.x = -1.5707960f;
        actor->shadowB.y = 0.0f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    case 3:
        t = (speed < 0.0f ? 1.0f - actor->field150 : actor->field150) * scale * 2.0f;
        actor->shadowB.x = 1.5707960f;
        actor->shadowB.y = 0.0f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    case 4:
        t = (speed < 0.0f ? actor->field150 : 1.0f - actor->field150) * scale * 2.0f;
        actor->shadowB.x = -0.7853980f;
        actor->shadowB.y = -1.5707960f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    case 5:
        t = (speed < 0.0f ? 1.0f - actor->field150 : actor->field150) * scale * 2.0f;
        actor->shadowB.x = 0.7853980f;
        actor->shadowB.y = 1.5707960f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    case 6:
        t = (speed < 0.0f ? actor->field150 : 1.0f - actor->field150) * scale * 2.0f;
        actor->shadowB.x = -2.3561940f;
        actor->shadowB.y = -1.5707960f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    case 7:
        t = (speed < 0.0f ? 1.0f - actor->field150 : actor->field150) * scale * 2.0f;
        actor->shadowB.x = 0.7853980f;
        actor->shadowB.y = -1.5707960f;
        actor->shadowA.y = t;
        actor->shadowA.w = 1.0f;
        actor->shadowB.z = 0.0f;
        actor->shadowA.x = 0.0f;
        actor->shadowA.z = 0.0f;
        break;
    default:
        break;
    }
    actor->shadowB.y += actor->rowB[1];
    Vu0AddVectorXYZ(&actor->shadowA, &actor->shadowA, &actor->position);
    Vu0AddVectorXYZ(&actor->shadowA, &actor->shadowA, &actor->localMatrix.m[3]);
}
#define XENO_DRAW_PARENT_FADE 0x4000000   /* fade by alphaScale */

/* XenoActorUpdateCameraFade: fades the actor at +0x188 by its distance to the active camera. */
float xglPointLength(Vector4 *from, Vector4 *to);
/* XenoActorUpdateCameraFade: fades the actor at +0x188 by its distance to the active camera. */
void XenoActorUpdateCameraFade(XenoActor *actor) {
    XglCamera *camera;
    PxyHeader *pack;
    DoublewordAlignedVector4 target;
    DoublewordVector4 cameraPosition;
    float radius;
    float distance;
    float fadeEnd;

    camera = xglStudioSelectGetActiveCamera(0);
    actor->alphaScale = 1.0f;
    if ((actor->drawFlags & XENO_DRAW_PARENT_FADE) && camera != 0 && actor->parent == 0 && (pack = actor->pack) != 0 &&
        pack->magic[0] == 'p' && pack->magic[1] == 'x' && pack->magic[2] == 'y' && (pack->flags & 0x81) == 0) {
        radius = pack->sphere.w;
        target = actor->rowA;
        cameraPosition = *(DoublewordVector4 *)&camera->travel.position;
        if (actor->fadeRadius != 0.0f) {
            radius = actor->fadeRadius;
        }
        Vu0AddVectorXYZ(&target, &target, &actor->position);
        target.w = 1.0f;
        distance = xglPointLength(&cameraPosition.vector, &target);
        if (distance < radius) {
            actor->alphaScale = 0.0f;
        } else {
            fadeEnd = radius * 1.6f;
            if (distance < fadeEnd) {
                actor->alphaScale = 1.0f - (fadeEnd - distance) / (fadeEnd - radius);
            }
        }
        if (actor->alphaScale < 0.0f) {
            actor->alphaScale = 0.0f;
        }
    }
}

/* XenoActor.flags bits: see the XENO_FLAG_* block after the XenoActor definition. */
#define XENO_DRAW_SOURCE_DIRTY 0x1000000

void xglVectorAdd(Vector4 *destination, const Vector4 *first, const Vector4 *second);
void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);
void xglCurSetRotateZ(f32 angle);
void tiMRPackageAdvanceA(void *handle);
void tiMRPackageAdvanceB(void *handle);
void tiMRPackageAdvanceAUnwrapped(void *handle);
void tiMRPackageAdvanceBUnwrapped(void *handle);

/* XenoActorUpdateTree: per-frame update of the actor and its child records (recursive). */
void XenoActorUpdateTree(XenoActor *actor) {
    XenoHrc *table;
    XenoMatrix inverseMatrix;
    Vector4 delta;
    DoublewordAlignedVector4 scaled;
    Vector4 angles;
    Vector4 jointDelta;
    u32 i;
    int child;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x47C);
    }
    if (actor != 0) {
        *(DoublewordAlignedVector4 *)actor->sourceBlock = actor->rowA;
        actor->drawFlags &= ~XENO_DRAW_SOURCE_DIRTY;
        if (actor->pack != 0 && actor->pack->magic[0] == 'p' && actor->pack->magic[1] == 'x' && actor->pack->magic[2] == 'y') {
            Vu0AddVectorXYZ(actor->sourceBlock, actor->sourceBlock, &actor->pack->sphere);
        }
        actor->renderValue += actor->renderValue2;
        XenoActorUpdateLinkMatrix(actor);
        if (actor->matrixBuf628 != 0) {
            XenoActorPushMatrix(actor);
            if (actor->table->index20 != 0 && (actor->flags & XENO_FLAG_SKIP_DELTA) == 0) {
                /* XenoMatrix is the actor's own view of the 4x4 matrix that xgl_2.h declares as the Matrix union. */
                xglMatrixInverse((Matrix *)&inverseMatrix, (Matrix *)&actor->localMatrix);
                xglVectorMulMat(&delta, inverseMatrix.m, (Vector4 *)actor->prevRowA);
                delta.x *= 0.3f;
                delta.y *= 0.3f;
                delta.z *= 0.3f;
                QuadwordCopy(&angles, actor->rowB);
                while (angles.x < 0.0f) {
                    angles.x += 6.2831855f;
                }
                while (angles.y < 0.0f) {
                    angles.y += 6.2831855f;
                }
                while (angles.z < 0.0f) {
                    angles.z += 6.2831855f;
                }
                scaled.z = actor->angle[2] - angles.z;
                scaled.x = actor->angle[0] - angles.x;
                scaled.y = actor->angle[1] - angles.y;
                xglCurSetRotateZ(scaled.z);
                xglCurMulRotateY(scaled.y);
                xglCurMulRotateX(scaled.x);
                for (i = 0; i < actor->table->index20; i++) {
                    xglVectorAdd((Vector4 *)actor->jointRecords[i].rest, (Vector4 *)actor->jointRecords[i].rest, &delta);
                    xglCurApplyVector(&jointDelta, &actor->jointRecords[i].translation.vector);
                    jointDelta.x = (jointDelta.x - actor->jointRecords[i].translation.vector.x) * 0.6f;
                    jointDelta.y = (jointDelta.y - actor->jointRecords[i].translation.vector.y) * 0.6f;
                    jointDelta.z = (jointDelta.z - actor->jointRecords[i].translation.vector.z) * 0.6f;
                    __asm__ __volatile__(
                        "lqc2 vf20, 0(%0)\n\t"
                        "lqc2 vf21, 0(%1)\n\t"
                        "vadd.xyzw vf20, vf20, vf21\n\t"
                        "sqc2 vf20, 0(%0)"
                        : : "r"(actor->jointRecords[i].rest + 0), "r"(&jointDelta) : "memory");
                }
                QuadwordCopy(actor->prevRowA, &actor->rowA);
                QuadwordCopy(actor->angle, &angles);
            } else {
                actor->flags &= ~XENO_FLAG_SKIP_DELTA;
            }
            if (actor->flags & XENO_FLAG_RESET_CHANNEL) {
                XenoActorResetChannelFlags(actor);
                actor->flags &= ~XENO_FLAG_RESET_CHANNEL;
            }
            tiMRStateSetPosition(&actor->flags650, actor->transformScale);
            tiMRStateSetPosition(&actor->flags6A0, actor->cachedValue);
            XenoActorApplyRequestedMode(actor);
            if (actor->flags & XENO_FLAG_SHIFT_JOINTS) {
                XenoActorShiftJoints(actor, 1, 1, 1, 1);
                actor->flags &= ~XENO_FLAG_SHIFT_JOINTS;
            }
            actor->position.x = actor->globalMatrices[1].m[3][0];
            actor->position.y = actor->globalMatrices[1].m[3][1];
            actor->position.z = actor->globalMatrices[1].m[3][2];
            actor->position.w = 1.0f;
        } else {
            /* Each arm resets the position with its own constants (kept in FPRs). */
            float zero;
            float one;

            if (actor->pamActive != 0) {
                tiMRStateSetPosition(&actor->flags650, actor->transformScale);
                tiMRStateSetPosition(&actor->flags6A0, actor->cachedValue);
                XenoActorApplyRequestedMode(actor);
                actor->flags &= ~XENO_FLAG_RESET_CHANNEL;
                zero = 0.0f;
                one = 1.0f;
                actor->position.z = zero;
                actor->position.y = zero;
                actor->position.x = zero;
                actor->position.w = one;
            } else {
                zero = 0.0f;
                one = 1.0f;
                actor->position.z = zero;
                actor->position.y = zero;
                actor->position.x = zero;
                actor->position.w = one;
            }
        }
        *(DoublewordAlignedVector4 *)actor->sourceBlock = actor->rowA;
        if ((actor->table != 0 && (actor->motionKeys[0] != 0 || actor->motionKeys[1] != 0)) || actor->pamActive != 0) {
            *(DoublewordAlignedVector4 *)actor->sourceBlock = actor->position;
            xglVectorScaleXYZ(&scaled, (Vector4 *)actor->sourceBlock, 1.0f / actor->rowC[0]);
            scaled.w = 1.0f;
            xglVectorMulMat((Vector4 *)actor->sourceBlock, actor->localMatrix.m, &scaled);
            QuadwordCopy(&scaled, actor->sourceBlock);
            /* Written back through the doubleword union view of the block. */
            ((DoublewordVector4 *)actor->sourceBlock)->vector = scaled;
            if ((actor->flags & XENO_FLAG_FREEZE_A) == 0) {
                if (actor->flags & XENO_FLAG_UNWRAP_A) {
                    tiMRPackageAdvanceAUnwrapped(actor->handle640);
                } else {
                    tiMRPackageAdvanceA(actor->handle640);
                }
                actor->transformScale = actor->motionTime;
            }
        }
        if (actor->table != 0 && (actor->motionKeys[2] != 0 || actor->motionKeys[3] != 0) && (actor->flags & XENO_FLAG_FREEZE_B) == 0) {
            if (actor->flags & XENO_FLAG_UNWRAP_B) {
                tiMRPackageAdvanceBUnwrapped(actor->handle640);
            } else {
                tiMRPackageAdvanceB(actor->handle640);
            }
            actor->cachedValue = actor->cachedMotionTime;
        }
        XenoActorUpdateMoveState(actor);
        XenoActorUpdateCameraFade(actor);
    }
    for (child = 0; child < 8; child++) {
        if (actor->children[child] != 0) {
            XenoActorUpdateTree(actor->children[child]);
        }
    }
}

extern unsigned char XenoActorItemTextIdle[];
extern unsigned char XenoActorItemTextActive[];

/* XenoActorRefreshItems: draws each table item; the active item uses its own text. */
void XenoActorRefreshItems(XenoActor *actor)
{
    XenoHrc *table;
    XenoActorItem *item;
    u32 itemIndex;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x52E);
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

    result = tiMRPackageGetNodeMatrix((int) actor->handle640);
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

    result = tiMRPackageGetNodePoint((int) actor->handle640);
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
void xglMatrixStackTrans(float *);
void xglMatrixStackRotX(float);
void xglMatrixStackRotZ(float);
void xglMatrixStackScale(float *);
void xglMatrixStackSave(float (*)[4]);
void xglMatrixStackLoad(float (*)[4]);
void xglMatrixStackMul(void *);

/* XenoActorPushMatrix: builds the actor transform on the matrix stack (translation, rotations, scale and offset of the actor or its parent), saves it in the local matrix and
   concatenates the override matrix; a slot of 2 or more additionally multiplies by the parent's joint matrices. */
void XenoActorPushMatrix(XenoActor *actor) {
    XenoActor *source;
    XenoMatrix *local = &actor->localMatrix;
    XenoMatrix *matrices;
    PxyHeader *pack;

    source = actor->parent;
    xglMatrixStackUnit();
    if (source == 0) {
        source = actor;
    }
    xglMatrixStackTrans(&source->rowA.x);
    xglMatrixStackRotZ(source->rowB[2]);
    xglMatrixStackRotY(source->rowB[1]);
    xglMatrixStackRotX(source->rowB[0]);
    xglMatrixStackScale(source->rowC);
    xglMatrixStackTrans(source->rowD);
    xglMatrixStackSave(local->m);
    if (actor->overrideMatrix != 0) {
        xglMatrixMul(local, local, actor->overrideMatrix);
    }
    if (actor->slotIndex >= 2 && actor->parent != 0 && actor->parent->globalMatrices != 0) {
        pack = actor->parent->pack;
        xglMatrixStackLoad(local->m);
        xglMatrixStackMul(actor->parent->globalMatrices + actor->slotIndex);
        xglMatrixStackMul((XenoMatrix *)((unsigned char *)pack + pack->jointMatrixOffset) + actor->slotIndex - 2);
        xglMatrixStackSave(local->m);
    }
}

void tiMRPackageSetFirstWord(void *package, int value);

void XenoActorSetFirstWord(struct XenoActor *actor, int value) {
    tiMRPackageSetFirstWord(actor->handle640, value);
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
/* XENO_DRAW_FOCUS_LOCKED (0x100) is declared in main/xenoActor.h. */
#define XENO_DRAW_FLAG_200 0x200
#define XENO_DRAW_NO_TREE_UPDATE 0x400    /* XenoActorDoit skips this root actor */
#define XENO_DRAW_ALPHA 0x10000
#define XENO_DRAW_RENDER_2 0x20000        /* adds render flag 2 */
#define XENO_DRAW_SHADOW_PROJECT 0x40000
#define XENO_DRAW_SHADOW_STENCIL 0x80000
#define XENO_DRAW_FLAG_100000 0x100000
#define XENO_DRAW_FLAG_200000 0x200000
#define XENO_DRAW_FLAG_800000 0x800000
#define XENO_DRAW_FLAG_2000000 0x2000000
#define XENO_DRAW_FLAG_8000000 0x8000000
#define XENO_DRAW_DREW 0x10000000         /* set when the pack drew something */
/* Any of these (or a shadow count) enters the shadow/focus block. */
#define XENO_DRAW_POST_MASK 0xA0C01C0
#define XENO_RENDER_FLAG_2 2
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
        actor->drawFlags &= ~XENO_DRAW_SKIP_ONCE;
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
        /* The actor is the joint state, and XenoMatrix is the matrix view the actor stores its global and local matrices as. */
        tiDrawJointDrawLists((struct TiDrawJointState *)actor, (float (*)[4][4])actor->globalMatrices, (float (*)[4][4])&actor->localMatrix);
        if (actor->attr & 4) {
            XenoActorRefreshItems(actor);
        }
    }
    if (actor != 0 && (pack = actor->pack) != 0 && pack->magic[0] == 'p' && pack->magic[1] == 'x' && pack->magic[2] == 'y') {
        localMatrix = &actor->localMatrix;
        if (actor->state != 0) {
            func_001BAA98(actor->state);
            actor->state = 0;
        }
        nmlModelSetDrawMatrix(localMatrix);
        pxySetModelScale(actor->rowC);
        if (actor->colorIndex != XENO_COLOR_INDEX_NONE) {
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
            if (actor->drawFlags & XENO_DRAW_LIGHT) {
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
        if (actor->drawFlags & XENO_DRAW_COLOR) {
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
        if (actor->flags & XENO_FLAG_CHANNEL_BIT20) {
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
                if (pack->sphere.w >= 3.0f) {
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
                pxySetClipPlaneWithAngle(&actor->shadowA, &actor->shadowB, actor->shadowC);
            } else if (actor->drawFlags & XENO_DRAW_SHADOW_STENCIL) {
                pxyAddClipPlanePair(&actor->shadowA, &actor->shadowB);
                pxyAddClipPlanePair(actor->shadowA2, actor->shadowB2);
                actor->drawFlags &= ~XENO_DRAW_SHADOW_STENCIL;
            }
            if (actor->shadowCount != 0) {
                if (pack->flags & 0x2000) {
                    geometry = (PxyGeometry *) ((char *) pack + pack->geometryOffset);
                    memset(actor->geometryMask, 0, sizeof(actor->geometryMask));
                    for (part = 0; part < pack->geometryCount; part++) {
                        if (geometry->flags & 0x100000) {
                            actor->geometryMask[part >> 5] |= 1 << (part & 0x1F);
                        }
                        geometry++;
                    }
                }
                pxySetClipPlaneMasked(&actor->shadowA, &actor->shadowB, actor->shadowCount, actor->geometryMask, actor->shadowValue,
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
            if (actor->drawFlags & XENO_DRAW_COLOR) {
                pxySetFogColor(actor->colorBlock);
                pxySetFogDist(actor->color[0], actor->color[1], actor->color[2], actor->color[3]);
            }
            if (actor->drawFlags & XENO_DRAW_LIGHT) {
                func_001BB96C(&actor->light);
            }
            if (actor->effect != 0) {
                pxySetTxyBlock(actor->effect);
            }
            func_001BB44C(actor->subVisibility);
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
                XenoActorRelease((XenoActor *)activeRecord);
            }
        } while (index < XenoActorPoolCount);
    }
}

void XenoActorDoit(void) {
    int i;

    if (XenoActorHoldFlag == 0) {
        for (i = 0; i < XenoActorPoolCount; ) {
            XenoActor *actor = (XenoActor *)XenoActorActiveTable[i++];
            if (actor != 0 && actor->parent == 0 && (actor->drawFlags & XENO_DRAW_NO_TREE_UPDATE) == 0) {
                XenoActorUpdateTree(actor);
            }
        }
    }
}

void ACT_DrawShadowBegin(void);
void ACT_DrawShadow(XenoActor *actor);
void ACT_DrawShadowEnd(void);

/* XenoActorDisp: draws every root actor of the active table, then the shadows when one of them uses draw mode 6 or 7. */
void XenoActorDisp(void) {
    int i;
    int drawShadows = 0;
    XenoActor *actor;
    func_001BBB18();
    for (i = 0; i < XenoActorPoolCount; i++) {
        actor = (XenoActor *)XenoActorActiveTable[i];
        if (actor != 0 && actor->parent == 0) {
            XenoActorDrawMotion(actor);
            actor = (XenoActor *)XenoActorActiveTable[i];
            if (actor->drawMode == 6 || actor->drawMode == 7) {
                drawShadows = 1;
            }
        }
    }
    if (drawShadows) {
        ACT_DrawShadowBegin();
        for (i = 0; i < XenoActorPoolCount; i++) {
            actor = (XenoActor *)XenoActorActiveTable[i];
            if (actor != 0) {
                ACT_DrawShadow(actor);
            }
        }
        ACT_DrawShadowEnd();
    }
}

/* XenoActorFindPart: index of the part with the given name (strcmp search). */
int strcmp(const char *, const char *);
int XenoActorFindPart(XenoActor *actor, const char *name) {
    PxyHeader *pack;
    PxyGeometry *geom;
    int i;

    if (actor != 0 && name != 0 && (pack = actor->pack) != 0 && pack->magic[0] == 'p' && pack->magic[1] == 'x' && pack->magic[2] == 'y') {
        geom = (PxyGeometry *)((unsigned char *)pack + pack->geometryOffset);
        for (i = 0; i < pack->geometryCount; i++) {
            if (strcmp((const char *)geom, name) == 0) {
                return i;
            }
            geom++;
        }
    }
    return -1;
}

/* XenoActorHidePart: hides a named part (sets its bit in the visibility word). */
int XenoActorHidePart(XenoActor *actor, const char *partName) {
    PxyHeader *pack;
    PxyHeader *subPack;
    PxyGeometry *geom;
    PxyGeometry *subGeom;
    int i;

    if (actor != 0 && partName != 0 && (pack = actor->pack) != 0 && pack->magic[0] == 'p' && pack->magic[1] == 'x' && pack->magic[2] == 'y') {
        geom = (PxyGeometry *)((unsigned char *)pack + pack->geometryOffset);
        if (pack->subPackOffset != 0) {
            subPack = (PxyHeader *)((unsigned char *)pack + pack->subPackOffset);
            subGeom = (PxyGeometry *)((unsigned char *)subPack + subPack->geometryOffset);
            for (i = 0; i < subPack->geometryCount; i++) {
                if (strcmp((const char *)subGeom, partName) == 0) {
                    actor->subVisibility[i >> 5] |= (1 << (i & 31));
                    return 1;
                }
                subGeom++;
            }
        }
        for (i = 0; i < pack->geometryCount; i++) {
            if (strcmp((const char *)geom, partName) == 0) {
                actor->visibility[i >> 5] |= (1 << (i & 31));
                return 1;
            }
            geom++;
        }
    }
    return 0;
}

/* XenoActorShowPart: shows a named part (clears its bit in the visibility word). */
int XenoActorShowPart(XenoActor *actor, const char *partName) {
    PxyHeader *pack;
    PxyHeader *subPack;
    PxyGeometry *geom;
    PxyGeometry *subGeom;
    int i;

    if (actor != 0 && partName != 0 && (pack = actor->pack) != 0 && pack->magic[0] == 'p' && pack->magic[1] == 'x' && pack->magic[2] == 'y') {
        geom = (PxyGeometry *)((unsigned char *)pack + pack->geometryOffset);
        if (pack->subPackOffset != 0) {
            subPack = (PxyHeader *)((unsigned char *)pack + pack->subPackOffset);
            subGeom = (PxyGeometry *)((unsigned char *)subPack + subPack->geometryOffset);
            for (i = 0; i < subPack->geometryCount; i++) {
                if (strcmp((const char *)subGeom, partName) == 0) {
                    actor->subVisibility[i >> 5] &= ~(1 << (i & 31));
                    return 1;
                }
                subGeom++;
            }
        }
        for (i = 0; i < pack->geometryCount; i++) {
            if (strcmp((const char *)geom, partName) == 0) {
                actor->visibility[i >> 5] &= ~(1 << (i & 31));
                return 1;
            }
            geom++;
        }
    }
    return 0;
}

void func_0028AF28(XenoActor *actor) {
    actor->flags |= XENO_FLAG_SHIFT_JOINTS;
}

/* XenoActorShiftJoints: adds or subtracts the actor offset to the translation of every joint. */
void XenoActorShiftJoints(XenoActor *actor, int sign, int x, int y, int z) {
    XenoHrc *table = actor->table;
    u32 i;

    if (table != 0) {
        if (sign > 0) {
            for (i = 0; i < table->index20; i++) {
                if (x) { actor->jointRecords[i].translation.vector.x += actor->position.x; }
                if (y) { actor->jointRecords[i].translation.vector.y += actor->position.y; }
                if (z) { actor->jointRecords[i].translation.vector.z += actor->position.z; }
            }
        } else {
            for (i = 0; i < table->index20; i++) {
                if (x) { actor->jointRecords[i].translation.vector.x -= actor->position.x; }
                if (y) { actor->jointRecords[i].translation.vector.y -= actor->position.y; }
                if (z) { actor->jointRecords[i].translation.vector.z -= actor->position.z; }
            }
        }
    }
}

/* XenoActorWrapRotation: copies the position and rotation vectors and wraps the three rotation angles into [0, 2*pi). */
void XenoActorWrapRotation(XenoActor *actor) {
    QuadwordCopy(&actor->prevRowA, &actor->rowA);
    QuadwordCopy(actor->angle, actor->rowB);
    while (actor->angle[0] < 0.0f) {
        actor->angle[0] += 6.2831855f;
    }
    while (actor->angle[1] < 0.0f) {
        actor->angle[1] += 6.2831855f;
    }
    while (actor->angle[2] < 0.0f) {
        actor->angle[2] += 6.2831855f;
    }
    actor->flags |= XENO_FLAG_SKIP_DELTA;
}

/* func_0028B158: moves the actor to its transformed position on the selected axes and shifts every joint record back by the actor position on those axes. */
void func_0028B158(XenoActor *actor, int moveX, int moveY, int moveZ) {
    XenoHrc *table = actor->table;
    Vector4 point;
    u32 i;

    XenoActorPushMatrix(actor);
    xglCurMatrixSet(actor->localMatrix.m[0]);
    xglCurApplyVector(&point, &actor->position);
    if (moveX) {
        actor->rowA.x = point.x;
    }
    if (moveY) {
        actor->rowA.y = point.y;
    }
    if (moveZ) {
        actor->rowA.z = point.z;
    }
    for (i = 0; i < table->index20; i++) {
        if (moveX) {
            actor->jointRecords[i].translation.vector.x -= actor->position.x;
        }
        if (moveY) {
            actor->jointRecords[i].translation.vector.y -= actor->position.y;
        }
        if (moveZ) {
            actor->jointRecords[i].translation.vector.z -= actor->position.z;
        }
    }
}

/* XenoActorSetFocusTarget: sets the actor's focus target point, shifts the previous point and draws the joint points when flag 0x100 is set.
   The two vector copies are the original's quadword-transfer inline asm (QuadwordCopy, include/quadword.h). */
void XenoActorSetFocusTarget(XenoActor *actor, Vector4 *target) {
    f32 matrix[4][4];

    tiWordSetBits(&actor->flags650, 0x10);
    QuadwordCopy(&actor->focusTarget, target);
    QuadwordCopy(&actor->focusPrevious, &actor->focusCurrent);
    if (actor->flags & XENO_FLAG_DRAW_POINTS) {
        xglCurSetTranslate(&actor->focusTarget);
        xglCurMatrixGet(matrix);
        tiDrawJointDrawPoints(matrix, 1.5f);
    }
}

void func_0028B2F0(XenoActor *actor, int value) {
    actor->handle640[1] = value;
}

/* XenoActorGetSlotRange: reads the two bounds of entry idx of the slot's range table as ints; 0 when the entry is missing or its bounds are inverted. */
int XenoActorGetSlotRange(XenoActor *actor, int slot, unsigned int idx, int *start, int *end) {
    XenoSlotTable *table;
    XenoSlotRange *range;

    if (actor == 0) {
        assertFailed(xenoActorAssertExpr, xenoActorAssertFile, 0x865);
    }
    table = actor->slotTable[slot];
    if (table == 0) {
        return 0;
    }
    if (idx < table->entryCount) {
        range = (XenoSlotRange *)((unsigned char *)table + *(table->entryOffset + idx));
        if (!(range->lower > range->upper)) {
            *start = (int)range->lower;
            *end = (int)range->upper;
            return 1;
        }
    }
    return 0;
}

/* XenoActorGetPoolCount: returns the pool count XenoActorPoolCount. */
int XenoActorGetPoolCount(void) {
    return XenoActorPoolCount;
}

/* Applies the actor's position through its local matrix to id. id is declared int although the callee takes a Vector4 *: the original passes the raw word (the original bytes treat it as an int argument). */
void XenoActorTransformOffset(struct XenoActor *actor, int id) {
    XenoActorPushMatrix(actor);
    xglCurMatrixSet(actor->localMatrix.m[0]);
    xglCurApplyVector((Vector4 *)id, &actor->position);
}
