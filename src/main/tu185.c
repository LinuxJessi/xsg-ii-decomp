#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
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

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefPacketUnpackRef);

void func_0012B6E0(XglPacket *packet, int, int, int, int);
void func_0012B770(XglPacket *packet);
void func_0012BCE8(XglPacket *packet, const void *data, int count);
extern unsigned char D_00881700[];
extern unsigned char D_00881740[];
extern unsigned char D_008817E0[];
extern unsigned char D_1F70[];
extern unsigned char D_1FD0[];

void sefBuildVuPacket(int kind, int alternate) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    if (kind == 0) {
        func_0012B6E0(packet, 0x14, 0x6C, 4, 4);
        func_0012BCE8(packet, D_00881700, 0x11);
    } else {
        if (alternate != 0) {
            func_0012B9A0(packet, ((u32) D_1FD0 >> 3) | 0x14000000);
        } else {
            func_0012B9A0(packet, ((u32) D_1F70 >> 3) | 0x14000000);
        }
        if (kind == 1) {
            func_0012B6E0(packet, 0x18, 0x6C, 4, 4);
            func_0012BCE8(packet, D_00881740, 0xD);
        } else {
            func_0012B6E0(packet, 0x22, 0x6C, 4, 4);
            func_0012BCE8(packet, D_008817E0, 3);
        }
    }
    func_0012B770(packet);
    func_0012B9A0(packet, 0x11000000);
}

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefBuildCameraPacket);

INCLUDE_ASM("asm/main/nonmatchings/tu185", func_0026C0A8);

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
    int handle;      /* +0x188: nonzero while the slot owns cells; also the value freed by smFreeHandle */
    u8 unmodeled_18c[0x14];
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

INCLUDE_ASM("asm/main/nonmatchings/tu185", sefClearSlotFlags);

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

void sefFreeSlotCells(void);

void sefReleaseCellSlot(ResourceSlot *slot) {
    if (slot != 0 && slot->handle != 0) {
        sefFreeSlotCells();
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
