#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
#include "quadword.h"
#include "gs.h"
#include "main/sm_initilize.h"
#include "main/tu185.h"

extern void *memset(void *destination, int value, unsigned int size);
extern unsigned char sefEntryPool[];

void sefResetPool(void) {
    memset(sefEntryPool, 0, 0x500B0);
}

extern unsigned char D_005E7AC0[];
void smInitilizeSmall(void *cell, unsigned int size);
void sefResetPool(void);

void sefResetBuffers(void) {
    smInitilizeSmall(D_005E7AC0, 0x20000);
    sefResetPool();
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefPoolAlloc);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026BD10);

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

/* VIF UNPACK code: CMD 0x6C (V4-32, USN set), NUM = count in bits 16-23, the low word 0x8000
 * fixed. packetSize (always 0x28 = sizeof(XglPacket) at every caller) is read nowhere here. */
#define VIF_UNPACK_V4_32_USN 0x6C008000U

void sefPacketUnpackRef(void *packet, int packetSize, int count, unsigned int value) {
    const void *environment;
    XglPacket *currentPacket;

    environment = (const void *) ((unsigned int) packet & 0x0FFFFFFF);
    currentPacket = xglPacketGetCurrent();
    sefPacketMscnt();
    ((u32 *) packet)[4] = value;
    ((u32 *) packet)[3] = VIF_UNPACK_V4_32_USN | ((unsigned int) (count - 1) << 16);
    sceVif1PkRef(currentPacket, environment, count, 0, 0, 0);
}

void func_0012B6E0(XglPacket *packet, int, int, int, int);
void func_0012B770(XglPacket *packet);
void func_0012BCE8(XglPacket *packet, const void *data, int count);
/* The VU packet at D_00881700: sefBuildVuPacket sends 0x11 quadwords from +0x00, 0xD from +0x40
 * or 3 from +0xE0. */
typedef struct SefVuPacket {
    u64 program[8];           /* +0x00 */
    Matrix4 modelViewMatrix;  /* +0x40: *sefViewMatrixPtr x the model matrix (func_0026C0A8) */
    Matrix4 cameraMatrix;     /* +0x80: copy of D_00831610 (func_0026C0A8) */
    u8 unmodeled_c0[0xe0 - 0xc0];
    u64 tex0;                 /* +0xE0: A+D TEX0_1 value */
    u64 tex0Register;         /* +0xE8: GS_TEX0_1 */
    u64 alpha;                /* +0xF0: A+D ALPHA_1 value */
    u64 alphaRegister;        /* +0xF8: GS_ALPHA_1 */
    u8 unmodeled_100[0x110 - 0x100];
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

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefBuildCameraPacket);

Matrix4 *MMathMulMatrix(Matrix4 *out, const Matrix4 *rhs, const Matrix4 *lhs);
extern const Matrix4 *sefViewMatrixPtr;
extern Matrix4 D_00831610;
/* GS ALPHA_1 values: [0] 0x44 normal blend, [1] 0x48 additive (rodata, also read by sdvDrawAlter). */
extern const u64 D_0068C5E8[];

/*
 * Fills the packet's model-view and camera matrices and its TEX0_1/ALPHA_1 A+D pairs: additive
 * blending for the blend kinds and flag values tested below, normal blending otherwise. Each
 * test keeps its own store, as the original's one block per test shows.
 */
void func_0026C0A8(u64 tex0, const Matrix4 *matrix, int blendKind, int flags) {
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

/* One 0x1A0-byte resource slot; only the fields the code touches are named. */
typedef struct ResourceSlot {
    u8 unmodeled_00[0x4];
    int flags;       /* +0x04: bit flags set by sefSetSlotFlags, cleared by sefClearSlotFlags */
    int state;       /* +0x08: nonzero while the slot is active */
    u8 unmodeled_0c[0x28];
    int key;         /* +0x34: the owner key that releases or flags the slot */
    u8 unmodeled_38[0x8];
    int resetFlag;   /* +0x40: set to 1 by sefReleaseCellSlot after the slot is cleared */
    u8 unmodeled_44[0x104];
    int link;        /* +0x148: nonzero while the slot is linked */
    u8 unmodeled_14c[0x3c];
    /*
     * +0x188: the address of this slot's cell-id table inside sefEntryPool, nonzero while
     * the slot owns cells. Declared `int` before sefFreeSlotCells.s showed sefReleaseCellSlot's
     * one other caller passing it straight to smFreeHandle(void *) with a pointer-from-integer
     * warning (sefReleaseCellSlot, unchanged instructions either way).
     */
    void *handle;
    int cellCountX;  /* +0x18C: with cellCountY, the cell-id table's element count (sefFreeSlotCells) */
    int cellCountY;  /* +0x190 */
    u8 unmodeled_194[0xc];
} ResourceSlot;

/* The table: a 0x40-byte header, then 192 slots. */
typedef struct ResourceTable {
    u8 unmodeled_00[0x40];
    ResourceSlot slots[192];
} ResourceTable;

extern ResourceTable sefResourceTable;

/* Clears the active count (sefActiveCount) and the whole slot table. */
void sefResetResourceTable(void) {
    sefActiveCount = 0;
    memset(&sefResourceTable, 0, 0x13800);
}

void sefReleaseByKey(int key);

void sefResetAll(void) {
    sefReleaseByKey(-1);
    sefResetBuffers();
    sefResetResourceTable();
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefReleaseByKey);

/* Sets flagBits in every active slot whose key matches; a negative key matches all. */
void sefSetSlotFlags(int key, int flagBits) {
    ResourceTable *table;
    int index;
    ResourceSlot *slot;

    table = &sefResourceTable;
    for (index = 0; index < 192; index++) {
        slot = &table->slots[index];
        if (slot->state != 0 && slot->link != 0 && (key < 0 || key == slot->key)) {
            slot->flags |= flagBits;
        }
    }
}

/* Clears flagBits in every active slot whose key matches; a negative key matches all. */
void sefClearSlotFlags(int key, int flagBits) {
    ResourceTable *table;
    int index;
    ResourceSlot *slot;

    table = &sefResourceTable;
    for (index = 0; index < 192; index++) {
        slot = &table->slots[index];
        if (slot->state != 0 && slot->link != 0 && (key < 0 || key == slot->key)) {
            slot->flags &= ~flagBits;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefFreeSlotCells);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026C488);

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefResetSlotRecords);

/* Same offsets as ResourceSlot: flags at +0x44, and the word at +0x188 that is nonzero for a slot with cells. */
typedef struct FlaggedObject {
    unsigned char unmodeled_00[0x44];
    int flags;
    unsigned char unmodeled_48[0x188 - 0x48];
    int enabled;
} FlaggedObject;

void sefObjectSetFlags(FlaggedObject *object, int mask) {
    if (object != 0 && object->enabled != 0) {
        object->flags |= mask;
    }
}

void sefObjectClearFlags(FlaggedObject *object, unsigned int mask) {
    if (object != 0 && object->enabled != 0) {
        object->flags &= ~mask;
    }
}

void sefFreeSlotCells(ResourceSlot *slot);

void sefReleaseCellSlot(ResourceSlot *slot) {
    if (slot != 0 && slot->handle != 0) {
        sefFreeSlotCells(slot);
        smFreeHandle(slot->handle);
        memset(slot, 0, sizeof(ResourceSlot));
        slot->resetFlag = 1;
    }
}

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

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026CEA8);

/* A rotation vector: x is the constant zero, y and z are the two angles. */
typedef struct RotationVector {
    int x;
    float y;
    float z;
} RotationVector;

void MMathRotateMatrixYXZ(u64 *matrix, int mode, const RotationVector *angles);
f32 sefRandAngle(void);

void sefRandomRotateMatrix(u64 *dest)
{
    RotationVector angles;
    u64 matrix[8];

    angles.x = 0;
    angles.y = sefRandAngle();
    angles.z = sefRandAngle();
    MMathRotateMatrixYXZ(matrix, 0, &angles);
    dest[0] = matrix[0];
    dest[1] = matrix[1];
    dest[2] = matrix[2];
    dest[3] = matrix[3];
    dest[4] = matrix[4];
    dest[5] = matrix[5];
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026CF90);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026D2B8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026D6A8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026D8C8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026DC18);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026DF30);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026E288);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026E5E0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026EA68);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026EC48);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026EDE8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F010);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F250);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F4C8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026F7D8);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026FB88);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026FFE0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00270428);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_002707C0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_002708E0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00270AE0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00270CA0);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00271078);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00271150);

void *sefGetResourceSlot(int index) {
    return (unsigned char *)&sefResourceTable + index * 0x1A0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefDrawResources);

/* The original body is a bare return: no code between entry and return. */
void func_00271390(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_00271398);
