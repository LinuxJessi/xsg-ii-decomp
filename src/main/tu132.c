#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu132", xglMatrixFromRows);

#include "shared.h"

typedef struct XglPacket XglPacket;

/*
 * The VIF1 packet builder object. main/tu059 and main/tu033 define its full
 * layout; this TU reads only the write cursor at +0x00.
 */
struct XglPacket {
    u32 *write_cursor;
    u8 unmodeled_04[0x24];
};

extern XglPacket *xglPacketGetCurrent(void);
/* Caches the packet every send in this TU writes to (xglPacketCaptureCurrent stores it). */
extern XglPacket *xglCurrentPacket;

/* Caches the current packet for the sends below. */
void xglPacketCaptureCurrent(void) {
    xglCurrentPacket = xglPacketGetCurrent();
}

void func_0012B6E0(XglPacket *packet, int id, int, int, int);
void sceVif1PkCnt(XglPacket *packet, int count);

/* Opens a VIF UNPACK (V4-32, code 0x6C) of the next data to VU1 address vuAddress. */
void xglPacketBeginUnpack(int vuAddress) {
    sceVif1PkCnt(xglCurrentPacket, 0);
    func_0012B6E0(xglCurrentPacket, vuAddress & 0xFFFF, 0x6C, 1, 1);
}

void func_0012B770(XglPacket *packet);

/* Ends the unpack opened by xglPacketBeginUnpack. */
void xglPacketEndUnpack(void) {
    func_0012B770(xglCurrentPacket);
}

void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode, int offset, int flags);
/* Count and entries of the reference table added by xglPacketRefTable. */
extern int xglPacketRefCount;
extern u8 D_00671E60[];

/* Adds a reference to the table D_00671E60 (xglPacketRefCount entries). */
void xglPacketRefTable(void) {
    sceVif1PkRef(xglCurrentPacket, &D_00671E60, xglPacketRefCount, 0, 0, 0);
}

void func_0012B9A0(XglPacket *packet, int tag);

/* Resets the count and emits one VIF FLUSH (0x11000000). */
void xglPacketFlush(void) {
    sceVif1PkCnt(xglCurrentPacket, 0);
    func_0012B9A0(xglCurrentPacket, 0x11000000);
}

/*
 * The block the frame sender passes to its helpers: two quadword rows at
 * +0x70 and +0x80 that are sent to the packet, and the matrix at +0x470 that
 * is loaded into the xgl matrix stack.
 */
typedef struct WaterFrameParams {
    u8 unmodeled_00[0x70];
    f32 row70[4];
    f32 row80[4];
    u8 unmodeled_90[0x3E0];
    Matrix4 stackMatrix;
} WaterFrameParams;

void func_0012BCE8(XglPacket *packet, const void *data, int count);
void xglMatrixStackMul(const void *data);
void xglMatrixStackLoad(float matrix[4][4]);
void xglMatrixStackSave(float matrix[4][4]);

/* Sends the stack matrix of params (the active camera tiWaterDrawItem passes) multiplied by the
 * object matrix at VU address 0x3C2; tiWaterSendMatrix sends the object matrix alone at 0x3C6. */
void tiWaterSendTransformed(WaterFrameParams *params, const void *matrix) {
    Matrix4 savedMatrix;

    xglMatrixStackLoad(&params->stackMatrix);
    xglMatrixStackMul(matrix);
    xglMatrixStackSave(savedMatrix);
    xglPacketBeginUnpack(0x3C2);
    func_0012BCE8(xglCurrentPacket, savedMatrix, 4);
    xglPacketEndUnpack();
}

/* Sends the four quads of data at VU address 0x3C6. */
void tiWaterSendMatrix(const void *data) {
    xglPacketBeginUnpack(0x3C6);
    func_0012BCE8(xglCurrentPacket, data, 4);
    xglPacketEndUnpack();
}

INCLUDE_ASM("asm/main/nonmatchings/tu132", func_0023F218);

void xglMatrixFromRows(float matrix[4][4], const float *row0, const float *row1, const float *row2,
                   const float *row3);
/* Sixteen floats: the four rows of the fixed matrix sent by tiWaterSendFixedMatrix. */
extern f32 D_00631800[16];

/* Sends the matrix built from D_00631800 at VU address 0x3CE. */
void tiWaterSendFixedMatrix(void) {
    Matrix4 matrix;

    xglMatrixFromRows(matrix, &D_00631800[0], &D_00631800[4], &D_00631800[8], &D_00631800[12]);
    xglPacketBeginUnpack(0x3CE);
    func_0012BCE8(xglCurrentPacket, matrix, 4);
    xglPacketEndUnpack();
}

/* Sets the constants 1/512 and 7/16 in the block, then sends row80 at 0x3C0 and row70 at 0x3C1. */
void tiWaterSendParamRows(WaterFrameParams *params) {
    params->row80[3] = 0.001953125f;
    params->row70[3] = 0.4375f;
    xglPacketBeginUnpack(0x3C0);
    func_0012BCE8(xglCurrentPacket, params->row80, 1);
    xglPacketEndUnpack();
    xglPacketBeginUnpack(0x3C1);
    func_0012BCE8(xglCurrentPacket, params->row70, 1);
    xglPacketEndUnpack();
}

INCLUDE_ASM("asm/main/nonmatchings/tu132", func_0023F3A0);

/* Sends the pair (x, y) at VU address 0x3DA. */
void func_0023F470(f32 x, f32 y) {
    f32 xy[2];

    xy[0] = x;
    xy[1] = y;
    xglPacketBeginUnpack(0x3DA);
    func_0012BCE8(xglCurrentPacket, xy, 1);
    xglPacketEndUnpack();
}

/* Sends the pair (x, y) at VU address 0x3DE. */
void func_0023F4B0(f32 x, f32 y) {
    f32 xy[2];

    xy[0] = x;
    xy[1] = y;
    xglPacketBeginUnpack(0x3DE);
    func_0012BCE8(xglCurrentPacket, xy, 1);
    xglPacketEndUnpack();
}

void func_0023F218(const void *data);

/* Sends one water frame: packet cache, reference table, transformed matrix, matrices, parameter rows, then the pair (0.7, 0.05). */
void tiWaterSendFrame(WaterFrameParams *params, const void *data) {
    xglPacketCaptureCurrent();
    xglPacketRefTable();
    tiWaterSendTransformed(params, data);
    tiWaterSendMatrix(data);
    func_0023F218(data);
    tiWaterSendFixedMatrix();
    tiWaterSendParamRows(params);
    func_0023F4B0(0.7f, 0.05f);
}

INCLUDE_ASM("asm/main/nonmatchings/tu132", tiWaterSendStrip);

INCLUDE_ASM("asm/main/nonmatchings/tu132", tiWaterSendGrid);
