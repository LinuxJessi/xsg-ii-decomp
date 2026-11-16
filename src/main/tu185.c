#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
#include "quadword.h"
#include "gs.h"
#include "main/sdv_033.h"
#include "main/sm_initilize.h"
#include "main/tu185.h"
#include "main/xgl_2.h"

extern void *memset(void *destination, int value, unsigned int size);

/* A 128-bit quadword (one sq), as sef_169.c spells it. */
typedef unsigned int Quadword __attribute__((mode(TI)));

/* One matrix row as the code writes it: single floats, a doubleword half (sd) or the whole quadword (sq). */
typedef union SefRotationRow {
    f32 element[4];
    u64 halves[2];
    Quadword quad;
} SefRotationRow;

/*
 * The 0xA0-byte particle of sefEntryPool has one layout per effect kind; the words every kind uses keep one
 * name in all three. sefPoolAlloc returns a particle's pool index, 0 when the pool is full.
 */

/* Kinds 0 to 7: spawned by func_0026CF90 (0 to 2), func_0026D2B8 (3 and 4) and sefSpawnKinds0To7 (5 to 7); kinds 0 to 2
 * drift in sefUpdateBurstParticles, kinds 5 to 7 grow and fade in sefUpdateFadeOutParticles. */
typedef struct SefFadeParticle {
    SefRotationRow rotation[3]; /* +0x00: built by sefRotationZRows */
    AlignedVector4 position;    /* +0x30 */
    int red;                    /* +0x40 */
    int green;                  /* +0x44 */
    int blue;                   /* +0x48 */
    int alpha;                  /* +0x4C */
    AlignedVector4 scale;       /* +0x50: xyz the axis scales, w the particle size */
    AlignedVector4 velocity;    /* +0x60: kinds 0 to 2: the step per frame for the first 20 frames */
    f32 life;                   /* +0x70: the particle dies once age reaches it; nonzero while the entry is in use */
    f32 age;                    /* +0x74: frames lived */
    f32 unmodeled_78;           /* zeroed on spawn */
    int unmodeled_7c;           /* zeroed on spawn */
    AlignedVector4 target;      /* +0x80: kinds 0 to 2: the step velocity turns towards after frame 20 */
    AlignedVector4 growth;      /* +0x90: per-axis factors around 1.0, w = 1.0 */
} SefFadeParticle;

/* Kinds 8 to 19 and 22 up: they fly along velocity at speed. */
typedef struct SefDriftParticle {
    SefRotationRow rotation[3]; /* +0x00: built by sefRotationZRows or sefRandomRotateMatrix */
    AlignedVector4 position;    /* +0x30 */
    int red;                    /* +0x40 */
    int green;                  /* +0x44 */
    int blue;                   /* +0x48 */
    int alpha;                  /* +0x4C */
    AlignedVector4 scale;       /* +0x50: xyz the axis scales, w the particle size */
    AlignedVector4 velocity;    /* +0x60: unit direction of flight (or already scaled by speed, sefSpawnKinds14To16) */
    f32 life;                   /* +0x70 */
    f32 age;                    /* +0x74 */
    f32 unmodeled_78;
    int unmodeled_7c;
    f32 speed;                  /* +0x80: sefSpawnKinds14To16 scales the unit velocity by it */
    int unmodeled_84;           /* zeroed on spawn */
    unsigned char unmodeled_88[0x90 - 0x88];
    AlignedVector4 growth;      /* +0x90 */
} SefDriftParticle;

/* Kinds 20 and 21: they turn about Z in place (sefUpdateKind20Particles, sefUpdateSpinParticles). */
typedef struct SefSpinParticle {
    SefRotationRow rotation[3]; /* +0x00: rebuilt from angle by sefRotationZRows */
    AlignedVector4 position;    /* +0x30 */
    int red;                    /* +0x40 */
    int green;                  /* +0x44 */
    int blue;                   /* +0x48 */
    int alpha;                  /* +0x4C */
    AlignedVector4 scale;       /* +0x50 */
    unsigned char unmodeled_60[0x70 - 0x60];
    f32 life;                   /* +0x70 */
    f32 age;                    /* +0x74 */
    f32 unmodeled_78;
    int unmodeled_7c;
    f32 angle;                  /* +0x80: degrees, 0 on spawn */
    unsigned char unmodeled_84[0x90 - 0x84];
    AlignedVector4 growth;      /* +0x90 */
} SefSpinParticle;

/* Any kind as sefPoolAlloc sees it: it tests the life word's bits (lw, beqz), zero (0.0f) for a free entry. */
typedef struct SefPoolRecord {
    u8 unmodeled_00[0x70];
    int life;                   /* +0x70 */
    u8 unmodeled_74[0xA0 - 0x74];
} SefPoolRecord;

/* One entry of sefEntryPool. The kind 8-21 spawners and the group updates reach the particle through the
 * union (a union access may alias anything, so gcc keeps the particle stores and the age reloads in the
 * original's order); sefSpawnKinds0To7 and sefUpdateSpinParticles take a pointer to their kind's member. */
typedef union SefPoolEntry {
    SefFadeParticle fade;
    SefDriftParticle drift;
    SefSpinParticle spin;
    SefPoolRecord record;
} SefPoolEntry;

/* The particle pool (0x500B0 bytes, cleared by sefResetPool). Entry 0 is never handed out: sefPoolAlloc returns
 * 0 when the pool is full. The SEF_POOL_RING_SIZE entries after it form a ring that the search for a free one walks. */
#define SEF_POOL_RING_SIZE 2048

typedef struct SefPool {
    SefPoolEntry entries[SEF_POOL_RING_SIZE + 1];
    int cursor;                 /* +0x500A0: where the next search starts */
} SefPool;

extern SefPool sefEntryPool;

void sefResetPool(void) {
    memset(&sefEntryPool, 0, sizeof(SefPool));
}

extern unsigned char D_005E7AC0[];
void smInitilizeSmall(void *cell, unsigned int size);

void sefResetBuffers(void) {
    smInitilizeSmall(D_005E7AC0, 0x20000);
    sefResetPool();
}

/* Set when sefPoolAlloc finds no free entry; sefRunEmitters clears it every frame. */
extern s16 D_0069DC48;

/* Returns the index of a free pool entry (ring[index] is entries[index + 1]), searching the ring from the cursor,
 * and leaves the cursor on it. Returns 0 and flags the pool full (D_0069DC48) when the search wraps around.
 * The free test reads life through SefPoolRecord: the plain `entry->fade.life == 0.0f` compiles to a float
 * compare (mtc1, lwc1, c.eq.s), the original tests the word (lw, beqz). The cursor is stored through poolCursor:
 * with `pool->cursor = ...` gcc folds the 0x500A0 offset into the store instead of keeping its address in a
 * register as the original does. */
int sefPoolAlloc(void) {
    SefPool *pool;
    SefPoolEntry *ring;
    SefPoolEntry *entry;
    int *poolCursor;
    int start;
    int last;
    int index;

    pool = &sefEntryPool;
    ring = &pool->entries[1];
    start = pool->cursor;
    poolCursor = &pool->cursor;
    last = (start - 1) & (SEF_POOL_RING_SIZE - 1);
    if (D_0069DC48 != 0) {
        return 0;
    }
    for (index = start; index != last; index &= SEF_POOL_RING_SIZE - 1) {
        entry = &ring[index++];
        if (entry->record.life == 0) {
            *poolCursor = index & (SEF_POOL_RING_SIZE - 1);
            return index;
        }
    }
    D_0069DC48 = 1;
    return 0;
}

/* VIF UNPACK code: CMD 0x6C (V4-32, USN set), NUM = count in bits 16-23, the low word 0x8000 fixed. */
#define VIF_UNPACK_V4_32_USN 0x6C008000U

/* The first data quadword of a block. The same 16 bytes are written at two widths: sefUnpackBlockInit clears the
 * whole quadword (sq) and then the count in its first word (sw), sefPacketUnpackRef writes the count (sw).
 * Flat fields (entryCount plus three zeroed words) compile to four sw instead of the original's por/sq. */
typedef union SefUnpackData {
    Quadword quad;
    unsigned int entryCount;  /* the particles in the block */
} SefUnpackData;

/* The head of a quadword block that func_00270CA0 builds and sends by reference: sefUnpackBlockInit zeroes the
 * first VIF codes and writes a one-quadword UNPACK, which sefPacketUnpackRef widens to the whole block. */
typedef struct SefUnpackBlock {
    unsigned int vifNop[3];   /* +0x00: zero VIF codes (NOP) */
    unsigned int unpackCode;  /* +0x0C: VIF UNPACK V4-32 of the block's data */
    SefUnpackData data;       /* +0x10 */
} SefUnpackBlock;

/* Starts a block: zeroes its VIF NOPs, writes a one-quadword UNPACK and clears the first data quadword and its
 * count. */
void sefUnpackBlockInit(SefUnpackBlock *block) {
    block->vifNop[0] = 0;
    block->vifNop[1] = 0;
    block->vifNop[2] = 0;
    block->unpackCode = VIF_UNPACK_V4_32_USN | (1 << 16);
    block->data.quad = 0;
    block->data.entryCount = 0;
}

typedef struct XglPacket XglPacket;

extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);
void func_0012B9A0(XglPacket *packet, unsigned int tag);

void sefPacketFlush(void) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
}

void sefPacketMscnt(void) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x17000000);
}

extern void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode, int offset, int flags);

/* blockSize (always 0x28 at every caller) is read nowhere here. */
void sefPacketUnpackRef(SefUnpackBlock *block, int blockSize, int count, unsigned int entryCount) {
    const void *environment;
    XglPacket *currentPacket;

    environment = (const void *) ((unsigned int) block & 0x0FFFFFFF);
    currentPacket = xglPacketGetCurrent();
    sefPacketMscnt();
    block->data.entryCount = entryCount;
    block->unpackCode = VIF_UNPACK_V4_32_USN | ((unsigned int) (count - 1) << 16);
    sceVif1PkRef(currentPacket, environment, count, 0, 0, 0);
}

void func_0012B6E0(XglPacket *packet, int, int, int, int);
void func_0012B770(XglPacket *packet);
void func_0012BCE8(XglPacket *packet, const void *data, int count);
/* The corners of a unit quad, (x, y, 0, 1) for x, y = -1, 1, as vectors and as the eight doublewords
 * sefBuildCameraPacket copies. */
typedef union SefQuadCorners {
    DoublewordAlignedVector4 corner[4];
    u64 words[8];
} SefQuadCorners;

/* The VU packet at D_00881700: sefBuildVuPacket sends 0x11 quadwords from +0x00, 0xD from +0x40
 * or 3 from +0xE0. */
typedef struct SefVuPacket {
    SefQuadCorners corners;   /* +0x00: copy of D_0068C610 (sefBuildCameraPacket) */
    Matrix4 modelViewMatrix;  /* +0x40: *sefViewMatrixPtr x the model matrix (sefBuildModelPacket) */
    Matrix4 cameraMatrix;     /* +0x80: copy of D_00831610 (sefBuildModelPacket) */
    u64 stripTag;             /* +0xC0: GIF tag: NLOOP 1, EOP, PRIM 0x54 (textured, blended triangle strip), NREG 13 */
    u64 stripRegisters;       /* +0xC8: its registers: A+D, then ST, RGBAQ and XYZ2 for four vertices */
    u64 settingsTag;          /* +0xD0: GIF tag of the three A+D pairs from tex0 on: NLOOP 3, EOP, NREG 1 */
    u64 settingsRegisters;    /* +0xD8: GIF_AD */
    u64 tex0;                 /* +0xE0: A+D TEX0_1 value */
    u64 tex0Register;         /* +0xE8: GS_TEX0_1 */
    u64 alpha;                /* +0xF0: A+D ALPHA_1 value */
    u64 alphaRegister;        /* +0xF8: GS_ALPHA_1 */
    u64 test;                 /* +0x100: A+D TEST_1 value (sefSetupEmitterPacket) */
    u64 testRegister;         /* +0x108: GS_TEST_1 */
} SefVuPacket;

extern SefVuPacket D_00881700;
extern unsigned char D_1F70[];
extern unsigned char D_1FD0[];

void sefBuildVuPacket(int kind, int alternate) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    if (kind == 0) {
        func_0012B6E0(packet, 0x14, 0x6C, 4, 4);
        func_0012BCE8(packet, &D_00881700, 0x11);
    } else {
        if (alternate != 0) {
            func_0012B9A0(packet, ((u32) D_1FD0 >> 3) | 0x14000000);
        } else {
            func_0012B9A0(packet, ((u32) D_1F70 >> 3) | 0x14000000);
        }
        if (kind == 1) {
            func_0012B6E0(packet, 0x18, 0x6C, 4, 4);
            func_0012BCE8(packet, &D_00881700.modelViewMatrix, 0xD);
        } else {
            func_0012B6E0(packet, 0x22, 0x6C, 4, 4);
            func_0012BCE8(packet, &D_00881700.tex0, 3);
        }
    }
    func_0012B770(packet);
    func_0012B9A0(packet, 0x11000000);
}

Matrix4 *MMathMulMatrix(Matrix4 *out, const Matrix4 *rhs, const Matrix4 *lhs);
extern const Matrix4 *sefViewMatrixPtr;
extern Matrix4 D_00831610;
XglCamera *xglStudioGetActiveCamera(void);
extern Matrix4 D_00493B40;
/* The unit quad (rodata). Declared without const: as a const object its loads cannot conflict with the packet stores,
 * and gcc then schedules the epilogue's ld $31 after the fourteen stores instead of before them (fd 53/70). */
extern SefQuadCorners D_0068C610;

/* Takes the active camera's view matrix (sefViewMatrixPtr) into the VU packet and D_00831610 (view x D_00493B40),
 * and fills the packet's quad corners, GIF tags and depth test. The corners are copied as doublewords through the
 * union: as one struct assignment the tag stores go out before the epilogue's ld $31 (fd 55/70). */
void sefBuildCameraPacket(void) {
    XglCamera *camera;

    camera = xglStudioGetActiveCamera();
    sefViewMatrixPtr = (const Matrix4 *) &camera->volume[0].viewMatrix;
    QuadwordCopyMatrixT0(&D_00881700.modelViewMatrix, sefViewMatrixPtr);
    MMathMulMatrix(&D_00831610, sefViewMatrixPtr, (const Matrix4 *) &D_00493B40);
    D_00881700.stripTag = 0xD02A400000008001ULL;
    D_00881700.stripRegisters = 0x000512512512512EULL;
    D_00881700.settingsTag = 0x1000000000008003ULL;
    D_00881700.settingsRegisters = GIF_AD;
    D_00881700.test = GS_SET_TEST(1, 6, 0, 0, 0, 0, 1, 2);
    D_00881700.testRegister = GS_TEST_1;
    D_00881700.corners.words[0] = D_0068C610.words[0];
    D_00881700.corners.words[1] = D_0068C610.words[1];
    D_00881700.corners.words[2] = D_0068C610.words[2];
    D_00881700.corners.words[3] = D_0068C610.words[3];
    D_00881700.corners.words[4] = D_0068C610.words[4];
    D_00881700.corners.words[5] = D_0068C610.words[5];
    D_00881700.corners.words[6] = D_0068C610.words[6];
    D_00881700.corners.words[7] = D_0068C610.words[7];
}

/* GS ALPHA_1 values: [0] 0x44 normal blend, [1] 0x48 additive (rodata, also read by sdvDrawAlter). */
extern const u64 D_0068C5E8[];

/*
 * Fills the packet's model-view and camera matrices and its TEX0_1/ALPHA_1 A+D pairs. Additive
 * blending for blend kinds 3-4; 5-7 unless flags bit 0 is set; 11-13 with flags 0 or 3; 14-16
 * with flags below 2; 17-19 with flags 2; 20 with flags 1. Normal blending otherwise. Each test
 * keeps its own store, as the original's one block per test shows.
 */
void sefBuildModelPacket(u64 tex0, const Matrix4 *matrix, int blendKind, int flags) {
    MMathMulMatrix(&D_00881700.modelViewMatrix, sefViewMatrixPtr, matrix);
    QuadwordCopyMatrixT0(&D_00881700.cameraMatrix, &D_00831610);
    D_00881700.tex0 = tex0;
    D_00881700.tex0Register = GS_TEX0_1;
    if (blendKind >= 3 && blendKind <= 4) {
        D_00881700.alpha = D_0068C5E8[1];
    } else if (blendKind >= 5 && blendKind <= 7) {
        if (flags & 1) {
            D_00881700.alpha = D_0068C5E8[0];
        } else {
            D_00881700.alpha = D_0068C5E8[1];
        }
    } else if (blendKind >= 11 && blendKind <= 13 && (flags == 0 || flags == 3)) {
        D_00881700.alpha = D_0068C5E8[1];
    } else if (blendKind >= 14 && blendKind <= 16 && flags < 2) {
        D_00881700.alpha = D_0068C5E8[1];
    } else if (blendKind >= 17 && blendKind <= 19 && flags == 2) {
        D_00881700.alpha = D_0068C5E8[1];
    } else if (blendKind == 20 && flags == 1) {
        D_00881700.alpha = D_0068C5E8[1];
    } else {
        D_00881700.alpha = D_0068C5E8[0];
    }
    D_00881700.alphaRegister = GS_ALPHA_1;
}

extern int sefActiveCount;

int sefGetActiveCount(void) {
    return sefActiveCount;
}

/* One 0x28-byte emitter of a resource slot: it spawns particles into its ring of pool indices
 * (D_005E79C8 per kind) and its group update ages them (D_005E7A30 per kind). */
typedef struct SefEmitter {
    u64 tex0;                  /* +0x00: GS TEX0_1 value of the emitter's image (sefSetupEmitterPacket) */
    unsigned short *particles; /* +0x08: ring of sefEntryPool indices, particleCount long; 0 for a free entry */
    f32 elapsed;               /* +0x0C: frames run; truncated to the frame number */
    f32 delay;                 /* +0x10: frames left before the emitter runs, counted down by sefRunEmitters */
    int frame;                 /* +0x14: the last whole frame handled; -1 until the first */
    short cursor;              /* +0x18: where the search for a free ring entry starts */
    unsigned short liveCount;  /* +0x1A: live particles counted by the last update */
    short spawned;             /* +0x1C: set by the emitters that spawn only once */
    u8 unmodeled_1e[0x28 - 0x1e];
} SefEmitter;

/* The part of a resource slot from +0x40 on (offsets below are slot offsets). sefSetSlotFlags walks the
 * table through this part: its loop pointer starts at sefResourceTable + 0x40. */
typedef struct ResourceBody {
    int resetFlag;   /* +0x40: set to 1 by sefReleaseCellSlot after the slot is cleared */
    int flags;       /* +0x44: bit flags (RESOURCE_FLAG_*), set by sefSetSlotFlags and sefObjectSetFlags */
    int state;       /* +0x48: nonzero while the slot is active */
    u8 unmodeled_4c[0x50 - 0x4c];
    /* +0x50: the owning actor or record (tu067.c passes pointers to sefSetRecordOwner); saved with the slot */
    void *owner;
    u8 unmodeled_54[0x58 - 0x54];
    /* +0x58: saved with the slot and written back by sefRestoreEffectRecords; no reader here. A pointer like owner:
     * as an int, sefCollectPersistentSlots's field copies come out in another order. */
    void *savedWord;
    u8 unmodeled_5c[0x6e - 0x5c];
    s16 visible;     /* +0x6E: set to 1 when the slot is set up; zero skips drawing */
    u8 unmodeled_70[0x74 - 0x70];
    int key;         /* +0x74: the owner key that releases or flags the slot */
    u8 unmodeled_78[0x80 - 0x78];
    Matrix4 matrix;  /* +0x80: the slot's model matrix, handed to sefBuildModelPacket */
    SefEmitter emitters[5]; /* +0xC0 */
    /* +0x188: this slot's particle-index rings inside the small-cell heap (particleCount entries per
     * emitter), nonzero while the slot owns cells; sefReleaseCellSlot frees it with smFreeHandle(void *). */
    unsigned short *handle;
    int particleCount; /* +0x18C: entries of each emitter's ring (a power of two) */
    int emitterCount;  /* +0x190: emitters in use: the loop bound of sefRunEmitters and sefDrawResources */
    s16 kind;          /* +0x194: effect kind: selects the handlers in D_005E79C8 and D_005E7A30 */
    s16 liveCount;     /* +0x196: live particles over all emitters, recounted by sefRunEmitters */
    u8 unmodeled_198[0x1a0 - 0x198];
} ResourceBody;

/* ResourceBody.flags bits read by sefRunEmitters (SUSPENDED is also set and cleared by sefResetSlotRecords). */
#define RESOURCE_FLAG_NO_DEPTH_TEST 0x4    /* sefSetupEmitterPacket draws the slot with the depth test off */
#define RESOURCE_FLAG_PAUSED 0x80          /* the spawn handler is skipped and an empty slot is kept */
#define RESOURCE_FLAG_SUSPENDED 0x400000   /* the slot is not updated at all */
#define RESOURCE_FLAG_PERSISTENT 0x4000000 /* an empty slot is kept */

/* One 0x1A0-byte resource slot of sefResourceTable; only the fields the code touches are named. */
typedef struct ResourceSlot {
    Vector4 position;  /* +0x00: w set to 1.0f per draw by sefDrawResources */
    Vector4 rotation;  /* +0x10 */
    Vector4 scale;     /* +0x20 */
    u8 unmodeled_30[0x40 - 0x30];
    ResourceBody body;
} ResourceSlot;

/* 192 slots (sefResetResourceTable clears 0x13800 bytes = 192 * 0x1A0). */
extern ResourceSlot sefResourceTable[192];

/* Clears the active count (sefActiveCount) and the whole slot table. */
void sefResetResourceTable(void) {
    sefActiveCount = 0;
    memset(sefResourceTable, 0, 0x13800);
}

void sefReleaseByKey(int key);

void sefResetAll(void) {
    sefReleaseByKey(-1);
    sefResetBuffers();
    sefResetResourceTable();
}

void sefReleaseCellSlot(ResourceSlot *slot);

/* Releases every active slot whose key matches (a negative key matches all), then clears the active count. */
void sefReleaseByKey(int key) {
    int index;

    for (index = 0; index < 192; index++) {
        if (sefResourceTable[index].body.state != 0 && sefResourceTable[index].body.handle != 0
            && (key < 0 || key == sefResourceTable[index].body.key)) {
            sefReleaseCellSlot(&sefResourceTable[index]);
        }
    }
    sefActiveCount = 0;
}

/* Sets flagBits in every active slot whose key matches; a negative key matches all. */
void sefSetSlotFlags(int key, int flagBits) {
    ResourceSlot *table;
    int index;
    ResourceBody *body;

    table = sefResourceTable;
    for (index = 0; index < 192; index++) {
        body = &table[index].body;
        if (body->state != 0 && body->handle != 0 && (key < 0 || key == body->key)) {
            body->flags |= flagBits;
        }
    }
}

/* Clears flagBits in every active slot whose key matches; a negative key matches all. */
void sefClearSlotFlags(int key, int flagBits) {
    ResourceSlot *table;
    int index;
    ResourceBody *body;

    table = sefResourceTable;
    for (index = 0; index < 192; index++) {
        body = &table[index].body;
        if (body->state != 0 && body->handle != 0 && (key < 0 || key == body->key)) {
            body->flags &= ~flagBits;
        }
    }
}

/* Frees every particle the slot's rings still hold: the pool entry's life goes to 0 and the ring entry with it. */
void sefFreeSlotCells(ResourceSlot *slot) {
    int i;

    for (i = 0; i < slot->body.particleCount * slot->body.emitterCount; i++) {
        if (slot->body.handle[i] != 0) {
            sefEntryPool.entries[slot->body.handle[i]].fade.life = 0.0f;
            slot->body.handle[i] = 0;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026C488);

/* Frees the particles of a slot that owns cells and restarts its emitters, then sets or clears its suspended flag. */
void sefResetSlotRecords(ResourceSlot *slot, int suspend) {
    int index;

    if (slot != 0 && slot->body.handle != 0) {
        sefFreeSlotCells(slot);
        for (index = 0; index < slot->body.emitterCount; index++) {
            slot->body.emitters[index].frame = -1;
            slot->body.emitters[index].elapsed = 0.0f;
            slot->body.emitters[index].delay = 0.0f;
        }
        if (suspend) {
            slot->body.flags |= RESOURCE_FLAG_SUSPENDED;
        } else {
            slot->body.flags &= ~RESOURCE_FLAG_SUSPENDED;
        }
    }
}

void sefObjectSetFlags(ResourceSlot *slot, int mask) {
    if (slot != 0 && slot->body.handle != 0) {
        slot->body.flags |= mask;
    }
}

void sefObjectClearFlags(ResourceSlot *slot, unsigned int mask) {
    if (slot != 0 && slot->body.handle != 0) {
        slot->body.flags &= ~mask;
    }
}

void sefReleaseCellSlot(ResourceSlot *slot) {
    if (slot != 0 && slot->body.handle != 0) {
        sefFreeSlotCells(slot);
        smFreeHandle(slot->body.handle);
        memset(slot, 0, sizeof(ResourceSlot));
        slot->body.resetFlag = 1;
    }
}

/* Picks the image pairs and emitter count for an effect kind (ids 0x100C83A7 and 0x100C83A8 force kinds 20
 * and 21) and creates its resource slot through func_0026C488. */
INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026C9C8);

f32 sefRandf(void);
void sefGetCirclePos(void *vector, f32 scale);

void sefGetCylinderPos(Vector4 *vector, f32 scale, f32 yScale) {
    sefGetCirclePos(vector, sefRandf() * scale);
    vector->y = sefRandf() * yScale;
}

void sefGetUnitDirection(AlignedVector4 *vector);

/* Scales the vector in place: by -scale when scale is negative, else by scale times sefRandf(). */
void sefScaleVectorRandom(AlignedVector4 *vector, f32 scale) {
    f32 factor;

    sefGetUnitDirection(vector);
    if (scale < 0.0f) {
        factor = -scale;
    } else {
        factor = sefRandf() * scale;
    }
    Vu0SefScaleVectorXYZNI(vector, vector, factor);
}

/* Writes a rotation about Z by angle into the first three rows of rows: (c, s, 0, 0), (-s, c, 0, 0), (0, 0, 1, 0).
 * VU0 macro mode: the cosine (0xE8) and sine (0x20) micro subroutines take the angle in vf4 and leave the result
 * in the x lane of vf1. Each result lands in the x member of a stack vector, as in the original; the two asm blocks
 * have no "memory" clobber, so the matrix stores reuse the FPR copies. */
void sefRotationZRows(SefRotationRow *rows, f32 angle) {
    Vector4 cosine;
    Vector4 sine;

    __asm__ __volatile__(
        "mfc1 $8, %1\n\t"
        "qmtc2.ni $8, vf4\n\t"
        "vcallms 0xE8\n\t"
        "qmfc2.i $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(cosine.x) : "f"(angle) : "$8");
    __asm__ __volatile__(
        "mfc1 $8, %1\n\t"
        "qmtc2.ni $8, vf4\n\t"
        "vcallms 0x20\n\t"
        "qmfc2.i $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(sine.x) : "f"(angle) : "$8");
    rows[0].element[0] = cosine.x;
    rows[0].element[1] = sine.x;
    rows[0].halves[1] = 0;
    rows[1].element[0] = -sine.x;
    rows[1].element[1] = cosine.x;
    rows[1].halves[1] = 0;
    rows[2].quad = 0;
    rows[2].element[2] = 1.0f;
}

Matrix4 *MMathRotateMatrixYXZ(Matrix4 *out, const Matrix4 *matrix, const Vector4 *angles);
f32 sefRandAngle(void);

void sefRandomRotateMatrix(u64 *dest)
{
    Vector4 angles;
    /* The rotation is copied out as doublewords (ld/sd), so the matrix is also seen as u64 words. */
    union {
        Matrix4 rows;
        u64 words[8];
    } matrix;

    angles.x = 0.0f;
    angles.y = sefRandAngle();
    angles.z = sefRandAngle();
    MMathRotateMatrixYXZ(&matrix.rows, 0, &angles);
    dest[0] = matrix.words[0];
    dest[1] = matrix.words[1];
    dest[2] = matrix.words[2];
    dest[3] = matrix.words[3];
    dest[4] = matrix.words[4];
    dest[5] = matrix.words[5];
}

/* Fires burst emitter index of slot once per frame: spawns up to count particles that fly from D_005E6EE0
 * towards a random point around D_005E6EF0. */
INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026CF90);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026D2B8);

/* What one emitter of an effect kind spawns (0x20 bytes). */
typedef struct SefEmitterParams {
    int interval;           /* +0x00: frames between spawns; 0 spawns every frame */
    int count;              /* +0x04: particles per spawn */
    f32 life;               /* +0x08 */
    f32 spread;             /* +0x0C: scale handed to sefScaleVectorRandom for the start position */
    f32 size;               /* +0x10 */
    f32 scaleW;             /* +0x14: stored in the particle's scale.w */
    short shade;            /* +0x18: red, green and blue */
    short alpha;            /* +0x1A */
    short lastFrame;        /* +0x1C: no spawning after this frame */
    unsigned char unmodeled_1e[0x20 - 0x1E];
} SefEmitterParams;

extern SefEmitterParams D_005E6D00[][4];
f32 sefNoiseNext(void);
void *MMathNormalizeVector2(void *destination, const Vector4 *source);

/* Emitters of effect kinds 0 to 7 (four per kind): once per frame, spawns up to count particles into free
 * entries of the ring. */
void sefSpawnKinds0To7(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefEmitterParams *params;
    SefFadeParticle *particle;
    int frame;
    int fire;
    int mask;
    int cursor;
    int last;
    int spawned;
    int id;
    int lastFrame;
    f32 growth;

    emitter = &slot->body.emitters[index];
    fire = 0;
    frame = emitter->elapsed;
    params = &D_005E6D00[slot->body.kind][index];
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        fire = 1;
    }
    if (emitter->frame != frame && frame <= lastFrame) {
        emitter->frame = frame;
        if (fire) {
            mask = slot->body.particleCount - 1;
            last = (emitter->cursor - 1) & mask;
            cursor = emitter->cursor;
            spawned = 0;
            while (cursor != last) {
                cursor &= mask;
                if (emitter->particles[cursor] == 0) {
                    id = sefPoolAlloc();
                    if (id == 0) {
                        break;
                    }
                    particle = &sefEntryPool.entries[id].fade;
                    emitter->particles[cursor] = id;
                    spawned++;
                    sefScaleVectorRandom(&particle->position, params->spread);
                    sefRotationZRows(particle->rotation, sefRandAngle());
                    particle->scale.x = particle->scale.y = particle->scale.z = params->size;
                    particle->scale.w = params->scaleW;
                    growth = sefNoiseNext() * 0.5f + 1.0f;
                    particle->growth.x = particle->growth.y = particle->growth.z = growth;
                    particle->growth.w = 1.0f;
                    particle->red = particle->green = particle->blue = params->shade;
                    particle->alpha = params->alpha;
                    particle->position.y += 0.25f;
                    particle->life = params->life;
                    particle->position.w = 1.0f;
                    particle->age = 0;
                    particle->unmodeled_78 = 0;
                    particle->unmodeled_7c = 0;
                    if (spawned >= params->count) {
                        break;
                    }
                }
                cursor++;
            }
        }
    }
}

/* Stores (1, 1, 1, 1) through vector: vf0.w broadcast into all four lanes of vf1. */
static inline void sefSetOnes(AlignedVector4 *vector) {
    __asm__ __volatile__(
        "vmaxw.xyzw vf1, vf0, vf0w\n"
        "sqc2 vf1, 0(%0)\n"
        :
        : "r" (vector)
        : "memory");
}

/* velocity = target - position (xyz): Vu0SefSubVectorXYZ with the position vector also named as the memory the
   asm reads. With the plain Vu0SefSubVectorXYZ (one mention of the pointer) gcc gives the callee-saved register
   before the parameter row to the row, after it to the position pointer, in all three spawners that use it. */
#define SefVelocityTowards(velocity, target, position) \
    __asm__ __volatile__( \
        "lqc2 vf1, 0(%1)\n" \
        "lqc2 vf2, 0(%2)\n" \
        "vsub.xyz vf1, vf1, vf2\n" \
        "sqc2 vf1, 0(%0)\n" \
        : \
        : "r" (velocity), "r" (target), "r" (position), "m" (*(position)) \
        : "memory")

extern SefEmitterParams D_005E7100[];

/* Emitters of effect kinds 8 to 10 (four per kind): 0 and 3 spin in place, 1 and 2 fly towards a random point
   above the origin; emitter 1 shrinks its particles after the ninth. */
void sefSpawnKinds8To10(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    AlignedVector4 *position;
    SefEmitterParams *params;
    AlignedVector4 direction;
    int frame;
    int due;
    int mask;
    int cursor;
    int last;
    int id;
    int spawned;
    int row;
    int lastFrame;

    due = 0;
    emitter = &slot->body.emitters[index];
    frame = emitter->elapsed;
    row = slot->body.kind - 8;
    params = &D_005E7100[index + row * 4];
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        due = 1;
    }
    if (emitter->frame == frame || lastFrame < frame) {
        return;
    }
    emitter->frame = frame;
    if (!due) {
        return;
    }
    spawned = 0;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] != 0) {
            continue;
        }
        id = sefPoolAlloc();
        if (id == 0) {
            break;
        }
        entry = &sefEntryPool.entries[id];
        position = &entry->drift.position;
        emitter->particles[cursor] = id;
        sefScaleVectorRandom(position, params->spread);
        entry->drift.scale.w = params->scaleW;
        entry->drift.growth.w = 1.0f;
        if (index == 0 || index == 3) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z =
                sefNoiseNext() * 0.5f + 1.0f;
        } else {
            sefRandomRotateMatrix((u64 *)entry->drift.rotation);
            if (index == 1 && spawned >= 9) {
                Vu0SetOrigin(&entry->drift.scale);
                entry->drift.scale.x = 1.0f;
                entry->drift.scale.y = 1.0f;
                entry->drift.scale.z = 1.0f;
                entry->drift.scale.w = 0.032f;
            } else {
                entry->drift.scale.x = 0.75f;
                entry->drift.scale.y = 3.0f;
                entry->drift.scale.z = 0.75f;
            }
            entry->drift.growth.x = sefNoiseNext() * params->size + 1.0f;
            entry->drift.growth.y = sefNoiseNext() * params->size + 1.0f;
            entry->drift.growth.z = sefNoiseNext() * params->size + 1.0f;
            sefScaleVectorRandom(&direction, -2.0f);
            direction.y += 1.5f;
            SefVelocityTowards(&entry->drift.velocity, &direction, position);
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
            entry->drift.speed = sefNoiseNext() * 0.025f + 0.15f;
            entry->drift.unmodeled_84 = 0;
        }
        entry->drift.position.y += 0.5f;
        entry->drift.position.w = 1.0f;
        entry->drift.red = entry->drift.green = entry->drift.blue = params->shade;
        entry->drift.alpha = params->alpha;
        entry->drift.life = params->life;
        entry->drift.age = 0;
        entry->drift.unmodeled_78 = 0;
        entry->drift.unmodeled_7c = 0;
        spawned++;
        if (spawned >= params->count) {
            break;
        }
    }
}

extern SefEmitterParams D_005E7280[];

/* Emitters of effect kinds 11 to 13 (five per kind): each spawn emits count particles, until lastFrame. */
void sefSpawnKinds11To13(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    SefEmitterParams *params;
    AlignedVector4 direction;
    int frame;
    int due;
    int mask;
    int cursor;
    int last;
    int id;
    int spawned;
    int row;
    int lastFrame;

    due = 0;
    emitter = &slot->body.emitters[index];
    row = slot->body.kind - 11;
    params = &D_005E7280[index + row * 5];
    frame = emitter->elapsed;
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        due = 1;
    }
    if (emitter->frame == frame || lastFrame < frame) {
        return;
    }
    emitter->frame = frame;
    if (!due) {
        return;
    }
    spawned = 0;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] != 0) {
            continue;
        }
        id = sefPoolAlloc();
        if (id == 0) {
            break;
        }
        entry = &sefEntryPool.entries[id];
        emitter->particles[cursor] = id;
        sefScaleVectorRandom(&entry->drift.position, params->spread);
        entry->drift.growth.w = 1.0f;
        entry->drift.scale.w = params->scaleW;
        if (index != 2) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z =
                sefNoiseNext() * 0.5f + 1.0f;
        } else {
            sefRandomRotateMatrix((u64 *)entry->drift.rotation);
            entry->drift.scale.x = 2.0f;
            entry->drift.scale.y = 5.0f;
            entry->drift.scale.z = 2.0f;
            entry->drift.growth.x = sefNoiseNext() * 1.5f + 1.0f;
            entry->drift.growth.y = sefNoiseNext() * 1.5f + 1.0f;
            entry->drift.growth.z = sefNoiseNext() * 1.5f + 1.0f;
            sefScaleVectorRandom(&direction, -2.0f);
            direction.y += 1.5f;
            Vu0SefSubVectorXYZ(&entry->drift.velocity, &direction, &entry->drift.position);
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
            entry->drift.speed = sefNoiseNext() * 0.05f + 0.2f;
            entry->drift.unmodeled_84 = 0;
        }
        entry->drift.position.y += 0.25f;
        entry->drift.position.w = 1.0f;
        entry->drift.red = entry->drift.green = entry->drift.blue = params->shade;
        entry->drift.alpha = params->alpha;
        entry->drift.life = params->life;
        entry->drift.age = 0;
        entry->drift.unmodeled_78 = 0;
        entry->drift.unmodeled_7c = 0;
        spawned++;
        if (spawned >= params->count) {
            break;
        }
    }
}

extern SefEmitterParams D_005E7460[];

/* Emitters 1 and 2 of these kinds add xglSRand() & SEF_SHADE_TINT_MASK, 0 to 31, to the shade. */
#define SEF_SHADE_TINT_MASK 0x1F

/* Emitters of effect kinds 22 and up (four per kind): like sefSpawnKinds8To10 with its emitters 1 and 2 facing a random
   angle, adding a random tint to the shade and emitter 2 shrinking its particles. */
void sefSpawnKinds22Up(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    AlignedVector4 *position;
    SefEmitterParams *params;
    AlignedVector4 direction;
    int frame;
    int due;
    int mask;
    int cursor;
    int last;
    int id;
    int spawned;
    int row;
    int lastFrame;
    int tint;

    due = 0;
    emitter = &slot->body.emitters[index];
    row = slot->body.kind - 22;
    params = &D_005E7460[index + row * 4];
    frame = emitter->elapsed;
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        due = 1;
    }
    if (emitter->frame == frame || lastFrame < frame) {
        return;
    }
    emitter->frame = frame;
    if (!due) {
        return;
    }
    spawned = 0;
    tint = 0;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] != 0) {
            continue;
        }
        id = sefPoolAlloc();
        if (id == 0) {
            break;
        }
        entry = &sefEntryPool.entries[id];
        position = &entry->drift.position;
        emitter->particles[cursor] = id;
        sefScaleVectorRandom(position, params->spread);
        entry->drift.scale.w = params->scaleW;
        entry->drift.growth.w = 1.0f;
        if (index == 0 || index == 3) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z =
                sefNoiseNext() * 0.5f + 1.0f;
        } else {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            tint = xglSRand() & SEF_SHADE_TINT_MASK;
            if (index == 2) {
                Vu0SetOrigin(&entry->drift.scale);
                entry->drift.scale.x = 1.0f;
                entry->drift.scale.y = 1.0f;
                entry->drift.scale.z = 1.0f;
                entry->drift.scale.w = 0.025f;
            } else {
                entry->drift.scale.x = 1.5f;
                entry->drift.scale.y = 1.5f;
                entry->drift.scale.z = 1.5f;
            }
            entry->drift.growth.x = sefNoiseNext() * params->size + 1.0f;
            entry->drift.growth.y = sefNoiseNext() * params->size + 1.0f;
            entry->drift.growth.z = sefNoiseNext() * params->size + 1.0f;
            sefScaleVectorRandom(&direction, -2.0f);
            direction.y += 1.5f;
            SefVelocityTowards(&entry->drift.velocity, &direction, position);
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
            entry->drift.speed = sefNoiseNext() * 0.05f + 0.21f;
            entry->drift.unmodeled_84 = 0;
        }
        entry->drift.position.y += 0.5f;
        entry->drift.position.w = 1.0f;
        entry->drift.red = entry->drift.green = entry->drift.blue = params->shade + tint;
        entry->drift.alpha = params->alpha;
        entry->drift.life = params->life;
        entry->drift.age = 0;
        entry->drift.unmodeled_78 = 0;
        entry->drift.unmodeled_7c = 0;
        spawned++;
        if (spawned >= params->count) {
            break;
        }
    }
}

extern SefEmitterParams D_005E75E0[];

/* Emitters of effect kinds 14 to 16 (five per kind): 0 to 2 fly outwards along their start position, 3 and 4 fly
   towards a random point. */
void sefSpawnKinds14To16(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    AlignedVector4 *position;
    SefEmitterParams *params;
    AlignedVector4 direction;
    float spread;
    int frame;
    int due;
    int mask;
    int cursor;
    int last;
    int id;
    int spawned;
    int row;
    int lastFrame;

    due = 0;
    emitter = &slot->body.emitters[index];
    row = slot->body.kind - 14;
    params = &D_005E75E0[index + row * 5];
    frame = emitter->elapsed;
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        due = 1;
    }
    if (emitter->frame == frame || lastFrame < frame) {
        return;
    }
    emitter->frame = frame;
    if (!due) {
        return;
    }
    spawned = 0;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] != 0) {
            continue;
        }
        id = sefPoolAlloc();
        if (id == 0) {
            break;
        }
        entry = &sefEntryPool.entries[id];
        position = &entry->drift.position;
        emitter->particles[cursor] = id;
        sefScaleVectorRandom(position, params->spread);
        entry->drift.scale.w = params->scaleW;
        entry->drift.growth.w = 1.0f;
        entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
        if (index < 3) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z =
                sefNoiseNext() * 0.5f + 1.0f;
            entry->drift.red = 0x50;
            entry->drift.green = 0x50;
            entry->drift.blue = 0x5A;
            if (index != 0) {
                entry->drift.speed = sefNoiseNext() * 0.005f + 0.005f;
            } else {
                entry->drift.speed = 0.001f;
            }
            MMathNormalizeVector2(&entry->drift.velocity, position);
            Vu0SefScaleVectorXYZNI(&entry->drift.velocity, &entry->drift.velocity, entry->drift.speed);
        } else {
            if (index == 3) {
                spread = 1.5f;
            } else {
                spread = 1.0f;
            }
            sefRandomRotateMatrix((u64 *)entry->drift.rotation);
            entry->drift.growth.x = sefNoiseNext() * spread + 1.0f;
            entry->drift.growth.y = sefNoiseNext() * spread + 1.0f;
            entry->drift.growth.z = sefNoiseNext() * spread + 1.0f;
            entry->drift.red = entry->drift.green = entry->drift.blue = params->shade;
            entry->drift.unmodeled_84 = 0;
            entry->drift.speed = sefNoiseNext() * 0.01f + 0.01f;
            sefScaleVectorRandom(&direction, -2.0f);
            SefVelocityTowards(&entry->drift.velocity, &direction, position);
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
        }
        entry->drift.position.y += 0.25f;
        entry->drift.alpha = params->alpha;
        entry->drift.unmodeled_7c = 0;
        entry->drift.position.w = 1.0f;
        entry->drift.age = 0;
        entry->drift.unmodeled_78 = 0;
        entry->drift.life = params->life;
        spawned++;
        if (spawned >= params->count) {
            break;
        }
    }
}

extern SefEmitterParams D_005E6D20[];

/* Emitters of effect kinds 17 to 19 (five per kind, D_005E6D20 indexed from kind 0): 0 and 1 start on a cylinder,
   2 at a random offset, 3 and 4 on a cylinder and fly towards a random point. */
void sefSpawnKinds17To19(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    SefEmitterParams *params;
    AlignedVector4 direction;
    int frame;
    int due;
    int mask;
    int cursor;
    int last;
    int id;
    int spawned;
    int lastFrame;

    due = 0;
    emitter = &slot->body.emitters[index];
    params = &D_005E6D20[index + slot->body.kind * 5];
    frame = emitter->elapsed;
    lastFrame = params->lastFrame;
    if (params->interval == 0 || frame % params->interval == 0) {
        due = 1;
    }
    if (emitter->frame == frame || lastFrame < frame) {
        return;
    }
    emitter->frame = frame;
    if (!due) {
        return;
    }
    spawned = 0;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] != 0) {
            continue;
        }
        id = sefPoolAlloc();
        if (id == 0) {
            break;
        }
        entry = &sefEntryPool.entries[id];
        emitter->particles[cursor] = id;
        entry->drift.scale.w = params->scaleW;
        entry->drift.growth.w = 1.0f;
        if (index == 2) {
            sefScaleVectorRandom(&entry->drift.position, params->spread);
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z = sefNoiseNext() + 1.0f;
            entry->drift.position.y = 0.2f;
        } else if (index == 1) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            sefGetCylinderPos(&entry->drift.position, 0.2f, 0.0f);
            entry->drift.position.y += 0.5f;
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z = sefNoiseNext() + 1.0f;
            entry->drift.velocity.x = sefNoiseNext() * 4.0f;
            entry->drift.velocity.y = sefRandf() * 2.0f;
            entry->drift.velocity.z = sefRandf() * 4.0f;
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
            entry->drift.speed = 0.06f;
        } else if (index == 0) {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            sefGetCylinderPos(&entry->drift.position, 0.2f, 0.0f);
            entry->drift.position.y += 0.5f;
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            entry->drift.growth.x = entry->drift.growth.y = entry->drift.growth.z = sefNoiseNext() + 1.0f;
        } else {
            sefRotationZRows(entry->drift.rotation, sefRandAngle());
            entry->drift.scale.x = entry->drift.scale.y = entry->drift.scale.z = params->size;
            if (index == 3) {
                entry->drift.growth.x = sefNoiseNext() + 1.0f;
                entry->drift.growth.y = sefNoiseNext() + 1.0f;
                entry->drift.growth.z = sefNoiseNext() + 1.0f;
                entry->drift.red = entry->drift.green = entry->drift.blue = 0x80;
            } else {
                entry->drift.growth.y = entry->drift.growth.z = entry->drift.growth.x = sefNoiseNext() + 1.0f;
                entry->drift.red = entry->drift.green = entry->drift.blue = 0x28;
            }
            sefGetCylinderPos(&entry->drift.position, 0.2f, 0.0f);
            entry->drift.position.y = 0.1f;
            sefScaleVectorRandom(&direction, -2.0f);
            direction.y += 1.0f;
            direction.z += 2.0f;
            Vu0SefSubVectorXYZ(&entry->drift.velocity, &direction, &entry->drift.position);
            MMathNormalizeVector2(&entry->drift.velocity, &entry->drift.velocity);
            entry->drift.speed = sefRandf() * 0.05f + 0.2f;
            entry->drift.unmodeled_84 = 0;
        }
        entry->drift.position.w = 1.0f;
        if (index < 3) {
            entry->drift.red = entry->drift.green = entry->drift.blue = params->shade;
        }
        entry->drift.alpha = params->alpha;
        if (slot->body.kind == 19) {
            entry->drift.position.z += emitter->elapsed * 0.2f;
        }
        spawned++;
        entry->drift.life = params->life;
        entry->drift.age = 0;
        entry->drift.unmodeled_78 = 0;
        entry->drift.unmodeled_7c = 0;
        if (spawned >= params->count) {
            break;
        }
    }
}

/* Per emitter of effect kind 20: the spawn interval in frames (also the particle life) and the particle size. */
extern short D_005E79A0[];
extern float D_005E79A8[];

/* Emitters of effect kind 20: one particle every interval frames; emitter 1 starts it orange-red and transparent. */
void sefSpawnKind20(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    int frame;
    int mask;
    int cursor;
    int last;
    int id;
    int due;

    emitter = &slot->body.emitters[index];
    frame = emitter->elapsed;
    due = frame % D_005E79A0[index] == 0;
    if (emitter->frame != frame) {
        emitter->frame = frame;
        if (!due) {
            return;
        }
        mask = slot->body.particleCount - 1;
        last = (emitter->cursor - 1) & mask;
        for (cursor = emitter->cursor; cursor != last; cursor++) {
            cursor &= mask;
            if (emitter->particles[cursor] == 0) {
                id = sefPoolAlloc();
                if (id == 0) {
                    break;
                }
                entry = &sefEntryPool.entries[id];
                emitter->particles[cursor] = id;
                sefSetOnes(&entry->spin.scale);
                sefSetOnes(&entry->spin.growth);
                entry->spin.scale.w = D_005E79A8[index];
                Vu0SetOrigin(&entry->spin.position);
                sefRotationZRows(entry->spin.rotation, 0.0f);
                if (index == 1) {
                    entry->spin.red = 200;
                    entry->spin.green = 60;
                    entry->spin.blue = 40;
                    entry->spin.alpha = 0;
                } else {
                    entry->spin.alpha = entry->spin.red = entry->spin.green = entry->spin.blue = 0x80;
                }
                entry->spin.life = D_005E79A0[index];
                entry->spin.angle = 0.0f;
                entry->spin.age = 0.0f;
                entry->spin.unmodeled_78 = 0.0f;
                entry->spin.unmodeled_7c = 0;
                break;
            }
        }
    }
}

/* Emitters of effect kind 21: one particle per emitter, once, on the first whole frame handled. */
void sefSpawnKind21(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    int frame;
    int mask;
    int cursor;
    int last;
    int id;

    emitter = &slot->body.emitters[index];
    frame = emitter->elapsed;
    if (emitter->frame == frame) {
        return;
    }
    emitter->frame = frame;
    if (emitter->spawned != 0) {
        return;
    }
    emitter->spawned = 1;
    mask = slot->body.particleCount - 1;
    last = (emitter->cursor - 1) & mask;
    for (cursor = emitter->cursor; cursor != last; cursor++) {
        cursor &= mask;
        if (emitter->particles[cursor] == 0) {
            id = sefPoolAlloc();
            if (id == 0) {
                break;
            }
            entry = &sefEntryPool.entries[id];
            emitter->particles[cursor] = id;
            sefSetOnes(&entry->spin.scale);
            sefSetOnes(&entry->spin.growth);
            if (index != 0) {
                entry->spin.scale.w = 0.32f;
            } else {
                entry->spin.scale.w = 0.64f;
            }
            Vu0SetOrigin(&entry->spin.position);
            sefRotationZRows(entry->spin.rotation, 0.0f);
            entry->spin.red = 200;
            entry->spin.life = 10.0f;
            entry->spin.unmodeled_7c = 0;
            entry->spin.blue = 0x80;
            entry->spin.green = 0x80;
            entry->spin.angle = 0.0f;
            entry->spin.age = 0.0f;
            entry->spin.unmodeled_78 = 0.0f;
            entry->spin.alpha = 0x80;
            break;
        }
    }
}

/* destination.xyz = from + (to - from) * t through vf1..vf3 (the w of to is replaced by 1 first). */
#define SefLerpVectorXYZ(destination, from, to, t) \
    __asm__ __volatile__( \
        "lqc2 vf1, 0(%1)\n" \
        "lqc2 vf2, 0(%2)\n" \
        "mfc1 $8, %3\n" \
        "vmove.w vf2, vf0\n" \
        "qmtc2.ni $8, vf3\n" \
        "vsub.xyz vf2, vf2, vf1\n" \
        "vmulx.xyz vf2, vf2, vf3x\n" \
        "vadd.xyz vf2, vf2, vf1\n" \
        "sqc2 vf2, 0(%0)\n" \
        : \
        : "r" (destination), "r" (from), "r" (to), "f" (t) \
        : "$8", "memory")

/* Group update of effect kinds 0 to 2 (D_005E7A30): ages the live particles and moves each by a step that turns
 * from velocity towards target after frame 20 and lengthens with age; the particles grow (fastest up to frame 10),
 * fade in until frame 10 and out from frame 60. */
void sefUpdateBurstParticles(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    AlignedVector4 step;
    unsigned short id;
    f32 age;
    int i;

    emitter = &slot->body.emitters[index];
    emitter->liveCount = 0;
    for (i = 0; i < slot->body.particleCount; i++) {
        id = emitter->particles[i];
        if (id != 0) {
            entry = &sefEntryPool.entries[id];
            if (entry->fade.age < entry->fade.life) {
                entry->fade.age = entry->fade.age + 1.0f;
                emitter->liveCount++;
                if (entry->fade.age > 20.0f) {
                    SefLerpVectorXYZ(&step, &entry->fade.velocity, &entry->fade.target,
                                     (entry->fade.age - 20.0f) * 0.01f);
                } else {
                    QuadwordCopyT0NoClobber(&step, &entry->fade.velocity);
                }
                Vu0SefScaleVectorXYZNI(&step, &step, entry->fade.age * 0.0008f + 0.025f);
                Vu0SefAddVectorXYZ(&entry->fade.position, &entry->fade.position, &step);
                age = entry->fade.age;
                if (age <= 10.0f) {
                    entry->fade.scale.x += 0.05f;
                } else if (age < 60.0f) {
                    entry->fade.scale.x += 0.03f;
                } else {
                    entry->fade.scale.x += 0.017f;
                }
                entry->fade.scale.z = entry->fade.scale.y = entry->fade.scale.x;
                if (age < 10.0f) {
                    entry->fade.alpha += 25;
                } else if (!(age < 60.0f)) {
                    entry->fade.alpha -= 4;
                    if (entry->fade.alpha < 0) {
                        entry->fade.alpha = 0;
                    }
                }
            } else {
                entry->fade.life = 0.0f;
                emitter->particles[i] = 0;
            }
        }
    }
    emitter->elapsed += 1.0f;
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F010);

/* Group update of effect kinds 5-7 (D_005E7A30): ages the live particles,
 * grows them and fades them out; odd emitters also brighten. */
void sefUpdateFadeOutParticles(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    unsigned short id;
    int i;

    emitter = &slot->body.emitters[index];
    emitter->liveCount = 0;
    for (i = 0; i < slot->body.particleCount; i++) {
        id = emitter->particles[i];
        if (id != 0) {
            entry = &sefEntryPool.entries[id];
            if (entry->fade.age < entry->fade.life) {
                entry->fade.age = entry->fade.age + 1.0f;
                emitter->liveCount++;
                if (index & 1) {
                    if (slot->body.kind == 5) {
                        if (entry->fade.age <= 10.0f) {
                            entry->fade.scale.x += 0.025f;
                        } else {
                            entry->fade.scale.x += 0.02f;
                        }
                    } else if (slot->body.kind == 6) {
                        if (entry->fade.age <= 10.0f) {
                            entry->fade.scale.x += 0.03f;
                        } else {
                            entry->fade.scale.x += 0.02f;
                        }
                    } else if (entry->fade.age <= 10.0f) {
                        entry->fade.scale.x += 0.07f;
                    } else {
                        entry->fade.scale.x += 0.033f;
                    }
                    entry->fade.scale.z = entry->fade.scale.y = entry->fade.scale.x;
                    entry->fade.green = entry->fade.blue = entry->fade.red = entry->fade.red + 1;
                    if (entry->fade.age <= 10.0f) {
                        entry->fade.alpha += 25;
                    } else if (entry->fade.age > 20.0f) {
                        entry->fade.alpha -= 12;
                    }
                    if (entry->fade.alpha < 0) {
                        entry->fade.alpha = 0;
                    }
                } else {
                    if (entry->fade.age <= 5.0f) {
                        entry->fade.scale.x += 0.16f;
                    } else {
                        entry->fade.scale.x += 0.06f;
                    }
                    entry->fade.scale.z = entry->fade.scale.y = entry->fade.scale.x;
                    if (entry->fade.age >= 10.0f) {
                        entry->fade.green -= 2;
                        entry->fade.blue -= 4;
                        entry->fade.alpha -= 6;
                        if (entry->fade.alpha < 0) {
                            entry->fade.alpha = 0;
                        }
                    }
                }
            } else {
                entry->fade.life = 0.0f;
                emitter->particles[i] = 0;
            }
        }
    }
    emitter->elapsed += 1.0f;
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F4C8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F7D8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026FB88);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026FFE0);

/* Group update of effect kind 20 (D_005E7A30): emitter 1 shifts the colour and
 * grows for 30 frames, emitter 2 turns 6 degrees a frame and pulses its size. */
void sefUpdateKind20Particles(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefPoolEntry *entry;
    unsigned short id;
    int i;

    emitter = &slot->body.emitters[index];
    emitter->liveCount = 1;
    for (i = 0; i < slot->body.particleCount; i++) {
        id = emitter->particles[i];
        if (id != 0) {
            entry = &sefEntryPool.entries[id];
            if (entry->spin.age < entry->spin.life) {
                if (index == 1) {
                    if (entry->spin.age <= 30.0f) {
                        entry->spin.green = 60 - (int) (entry->spin.age + entry->spin.age);
                        entry->spin.red = (int) (entry->spin.age * 1.667f) + 200;
                        entry->spin.blue = (int) (entry->spin.age * 1.333f) + 40;
                    }
                    if (entry->spin.age <= 16.0f) {
                        entry->spin.alpha += 8;
                    } else if (entry->spin.age <= 31.0f) {
                        entry->spin.alpha -= 8;
                    }
                    if (entry->spin.age <= 30.0f) {
                        entry->spin.scale.y = entry->spin.scale.z = entry->spin.scale.x =
                            entry->spin.scale.x + 0.0333f;
                    }
                }
                if (index == 2) {
                    entry->spin.angle -= 6.0f;
                    sefRotationZRows(entry->spin.rotation, entry->spin.angle * 0.017453292f);
                    if (entry->spin.age < 18.0f) {
                        entry->spin.scale.x += 0.0222f;
                    } else if (entry->spin.age < 22.0f) {
                        entry->spin.scale.x += 0.025f;
                    } else if (entry->spin.age < 25.0f) {
                        entry->spin.scale.x -= 0.2f;
                    } else if (entry->spin.age < 30.0f) {
                        entry->spin.scale.x += 0.02f;
                    } else if (entry->spin.age < 48.0f) {
                        entry->spin.scale.x += 0.0222f;
                    } else if (entry->spin.age < 52.0f) {
                        entry->spin.scale.x += 0.025f;
                    } else if (entry->spin.age < 55.0f) {
                        entry->spin.scale.x -= 0.2f;
                    } else if (entry->spin.age < 60.0f) {
                        entry->spin.scale.x += 0.02f;
                    }
                    entry->spin.scale.y = entry->spin.scale.x;
                }
                entry->spin.age = entry->spin.age + 1.0f;
            } else {
                entry->spin.life = 0.0f;
                emitter->particles[i] = 0;
            }
        }
    }
    emitter->elapsed += 1.0f;
    if (emitter->elapsed > 59.9f) {
        emitter->elapsed = 0.0f;
    }
}

/* Counts the live particles of the emitter; except for emitter 0 each one also turns 6 degrees (back to 0 once
 * at or past -359) and gets its matrix rebuilt. Then the emitter ages by one frame. */
void sefUpdateSpinParticles(ResourceSlot *slot, int index) {
    SefEmitter *emitter;
    SefSpinParticle *particle;
    int i;
    unsigned short id;

    emitter = &slot->body.emitters[index];
    emitter->liveCount = 0;
    for (i = 0; i < slot->body.particleCount; i++) {
        id = emitter->particles[i];
        if (id != 0) {
            particle = &sefEntryPool.entries[id].spin;
            if (index != 0) {
                particle->angle -= 6.0f;
                if (particle->angle <= -359.0f) {
                    particle->angle = 0.0f;
                }
                sefRotationZRows(particle->rotation, particle->angle * 0.017453292f);
            }
            emitter->liveCount++;
        }
    }
    emitter->elapsed += 1.0f;
}

/* A per-kind emitter handler, called with the slot and the emitter index. */
typedef void (*SefEmitterFunc)(ResourceSlot *slot, int index);

/* The owner record of sef_164.c; a resource slot is one (its key word at +0x74). */
typedef struct SefOwner SefOwner;

void uwamonoReleaseOwnedRecords(SefOwner *owner);
extern s16 sefClipTestDisabled;
extern f32 D_0069E334;
extern SefEmitterFunc D_005E79C8[];
extern SefEmitterFunc D_005E7A30[];

/* Runs the emitters of every active slot (or only of target when it is not null) and recounts sefActiveCount:
 * an emitter waits out its delay first, then runs its kind's spawn and group-update handlers. A slot left
 * without live particles is released unless it is paused or persistent. */
void sefRunEmitters(ResourceSlot *target) {
    ResourceSlot *slot;
    SefEmitter *emitter;
    int index;
    int emitterIndex;

    sefActiveCount = 0;
    D_0069DC48 = 0;
    if (sefClipTestDisabled != 0) {
        return;
    }
    for (index = 0; index < 192; index++) {
        slot = &sefResourceTable[index];
        if (slot->body.state == 0) {
            continue;
        }
        if (target != 0 && target != slot) {
            continue;
        }
        if (slot->body.flags & RESOURCE_FLAG_SUSPENDED) {
            continue;
        }
        D_0069E334 = (0.0f < slot->position.y) ? -slot->position.y : 0.0f;
        slot->body.liveCount = 0;
        for (emitterIndex = 0; emitterIndex < slot->body.emitterCount; emitterIndex++) {
            emitter = &slot->body.emitters[emitterIndex];
            if (0.0f < emitter->delay) {
                emitter->delay -= 1.0f;
            } else {
                if (!(slot->body.flags & RESOURCE_FLAG_PAUSED)) {
                    D_005E79C8[slot->body.kind](slot, emitterIndex);
                }
                D_005E7A30[slot->body.kind](slot, emitterIndex);
                slot->body.liveCount += emitter->liveCount;
            }
        }
        sefActiveCount += slot->body.liveCount;
        if (slot->body.liveCount == 0
            && !(slot->body.flags & (RESOURCE_FLAG_PERSISTENT | RESOURCE_FLAG_PAUSED))) {
            if (((slot->body.key >> 28) & 3) == 1) {
                uwamonoReleaseOwnedRecords((SefOwner *)slot);
            }
            sefReleaseCellSlot(slot);
        }
    }
}

Matrix4 *MMathRotateMatrixXYZ(Matrix4 *out, const Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathScaleMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *scale);
extern const Matrix4 MMathUnitMatrix;

/* Loads MMathUnitMatrix into destination: rows 0..2 through vf3..vf1, row 3 stored from vf0 (0, 0, 0, 1). */
static inline void sefLoadUnitMatrix(Matrix4 *destination) {
    __asm__ __volatile__(
        "lqc2 vf1, 32(%1)\n"
        "lqc2 vf2, 16(%1)\n"
        "lqc2 vf3, 0(%1)\n"
        "sqc2 vf0, 48(%0)\n"
        "sqc2 vf1, 32(%0)\n"
        "sqc2 vf2, 16(%0)\n"
        "sqc2 vf3, 0(%0)\n"
        :
        : "r" (destination), "r" (&MMathUnitMatrix)
        : "memory");
}

/* Nonzero when x, y or z has any bit set: the original tests the raw words (x and y as one doubleword, z as a
 * word), so -0.0f counts as set. */
static inline short sefVectorXYZSet(const Vector4 *vector) {
    return (*(const int *)&vector->z | *(const u64 *)&vector->x) != 0;
}

/* Sets the VU packet up for one emitter of slot: with the slot matrix, or, when the slot has a position, rotation
 * or scale of its own, with that transform applied to it first (and the camera matrix scaled too); then TEST_1,
 * whose depth test is off (ZTST ALWAYS) under RESOURCE_FLAG_NO_DEPTH_TEST or for kinds 20 and 21, else GEQUAL.
 * tex0 and kind, the caller's copies of emitter->tex0 and slot->body.kind, are not read. The (const Matrix4 *)
 * casts only silence gcc's warning for a Matrix4 * passed as const Matrix4 *. */
void sefSetupEmitterPacket(u64 tex0, ResourceSlot *slot, short kind, int emitterIndex) {
    Matrix4 matrix;
    SefEmitter *emitter;
    short hasPosition;
    short hasRotation;
    short hasScale;

    emitter = &slot->body.emitters[emitterIndex];
    hasPosition = sefVectorXYZSet(&slot->position);
    hasRotation = sefVectorXYZSet(&slot->rotation);
    hasScale = sefVectorXYZSet(&slot->scale);
    if (hasPosition || hasRotation || hasScale) {
        sefLoadUnitMatrix(&matrix);
        if (hasPosition) {
            QuadwordCopyT0(matrix[3], &slot->position);
        }
        if (hasRotation) {
            MMathRotateMatrixXYZ(&matrix, (const Matrix4 *) &matrix, &slot->rotation);
        }
        if (hasScale) {
            MMathScaleMatrix(&matrix, &matrix, &slot->scale);
        }
        MMathMulMatrix(&matrix, (const Matrix4 *) &matrix, (const Matrix4 *) &slot->body.matrix);
        sefBuildModelPacket(emitter->tex0, (const Matrix4 *) &matrix, slot->body.kind, emitterIndex);
        MMathScaleMatrix(&D_00881700.cameraMatrix, &D_00881700.cameraMatrix, &slot->scale);
    } else {
        sefBuildModelPacket(emitter->tex0, (const Matrix4 *) &slot->body.matrix, slot->body.kind, emitterIndex);
    }
    if (slot->body.flags & RESOURCE_FLAG_NO_DEPTH_TEST) {
        D_00881700.test = GS_SET_TEST(1, 6, 0, 0, 0, 0, 0, 1);
    } else if (slot->body.kind == 20 || slot->body.kind == 21) {
        D_00881700.test = GS_SET_TEST(1, 6, 0, 0, 0, 0, 0, 1);
    } else {
        D_00881700.test = GS_SET_TEST(1, 6, 0, 0, 0, 0, 1, 2);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00270CA0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00271078);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00271150);

void *sefGetResourceSlot(int index) {
    return (unsigned char *)&sefResourceTable + index * 0x1A0;
}

void func_00258970(void);
void svBuildDrawStatePacket(void);
void func_00270CA0(ResourceSlot *slot, int emitterIndex);
extern int D_0069E330;

/* Rebuilds the draw-state and VU packets, then draws every emitter of slot
 * count (one call per emitter, up to emitterCount), unless the active count,
 * the slot count or the slot's final kind say otherwise. */
void sefDrawResources(int count) {
    ResourceSlot *slot;
    int emitterIndex;

    if (sefActiveCount <= 0) {
        return;
    }
    if ((unsigned int)count >= 0xC0) {
        return;
    }
    D_0069E330 = -1;
    func_00258970();
    svBuildDrawStatePacket();
    sefBuildCameraPacket();
    sefBuildVuPacket(0, 0);

    slot = &sefResourceTable[count];
    emitterIndex = 0;
    if (slot->body.emitterCount > 0) {
        do {
            slot->position.w = 1.0f;
            func_00270CA0(slot, emitterIndex);
            emitterIndex += 1;
            D_0069E330 = slot->body.kind;
        } while (emitterIndex < slot->body.emitterCount);
    }
    if (D_0069E330 >= 0) {
        sefPacketFlush();
    }
}

/* The original body is a bare return: no code between entry and return. */
void func_00271390(void) {
}

/* One entry of sefEffectRecordList (tu176.c, 64 entries): a persistent slot saved by sefCollectPersistentSlots and
 * respawned by sefRestoreEffectRecords, which passes position, rotation, scale, key and slotIndex to
 * sefSpawnEffectRecord and writes owner, savedWord, flags and visible back into the new record. */
typedef struct SefSavedEffect {
    Vector4 position;  /* +0x00 */
    Vector4 rotation;  /* +0x10 */
    Vector4 scale;     /* +0x20 */
    void *owner;       /* +0x30: the slot's owner */
    void *savedWord;   /* +0x34: the slot's savedWord */
    int key;           /* +0x38 */
    int flags;         /* +0x3C */
    s16 visible;       /* +0x40 */
    s16 resetFlagLow;  /* +0x42: not read back */
    s16 slotIndex;     /* +0x44: index in sefResourceTable */
    u8 unmodeled_46[0x50 - 0x46];
} SefSavedEffect;

/* Appends every active persistent slot to the saved list from entry count on, up to 64 entries (sefCollectEffectRecords
 * tail-calls it after collecting the effect records). */
void sefCollectPersistentSlots(SefSavedEffect *list, int count) {
    ResourceSlot *slot;
    int index;

    slot = sefResourceTable;
    for (index = 0; index < 192; index++, slot++) {
        if (slot->body.state != 0 && count < 64 && (slot->body.flags & RESOURCE_FLAG_PERSISTENT)) {
            QuadwordCopyT0(&list[count].position, &slot->position);
            QuadwordCopyT0(&list[count].rotation, &slot->rotation);
            QuadwordCopyT0(&list[count].scale, &slot->scale);
            list[count].flags = slot->body.flags;
            list[count].owner = slot->body.owner;
            list[count].key = slot->body.key;
            list[count].savedWord = slot->body.savedWord;
            list[count].visible = slot->body.visible;
            list[count].resetFlagLow = slot->body.resetFlag;
            list[count].slotIndex = index;
            count++;
        }
    }
}
