#include "common.h"

/* Fills the four rows of matrix from row0..row3 (matrix[r][i] = row r [i]). */
void xglMatrixFromRows(float matrix[4][4], const float *row0, const float *row1, const float *row2,
                       const float *row3) {
    unsigned int i;

    for (i = 0; i < 4; i++) {
        float *dst = &matrix[0][i];
        const float *src = &row0[i];
        *dst = *src;
    }
    for (i = 0; i < 4; i++) {
        float *dst = &matrix[1][i];
        const float *src = &row1[i];
        *dst = *src;
    }
    for (i = 0; i < 4; i++) {
        float *dst = &matrix[2][i];
        const float *src = &row2[i];
        *dst = *src;
    }
    for (i = 0; i < 4; i++) {
        float *dst = &matrix[3][i];
        const float *src = &row3[i];
        *dst = *src;
    }
}

#include "shared.h"

/* VIF command words: UNPACK (V4-32), MSCNT (0x17) that starts the VU1 micro-program, and FLUSH. */
#define VIF_CODE_UNPACK_V4_32 0x6C
#define VIF_CODE_MSCNT 0x17000000
#define VIF_CODE_FLUSH 0x11000000
/* Scratchpad addresses: the GIF tag, and the quadword rows that follow it. */
#define SCRATCH_GIF_TAG 0x70000000
#define SCRATCH_QUAD_ROWS 0x70000010
/* GIF tag EOP bit (bit 15 of the tag's low halfword), kept when the tag's loop count is set. */
#define GIF_TAG_EOP 0x8000
/* VU1 addresses of the water sends (the vuAddress argument of xglPacketBeginUnpack). */
#define TIWATER_VU_ROW80 0x3C0
#define TIWATER_VU_ROW70 0x3C1
#define TIWATER_VU_TRANSFORMED 0x3C2
#define TIWATER_VU_MATRIX 0x3C6
#define TIWATER_VU_ORIENTED 0x3CA
#define TIWATER_VU_FIXED 0x3CE
#define TIWATER_VU_POSITION 0x3D2
#define TIWATER_VU_DIRECTION 0x3D6

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
    func_0012B6E0(xglCurrentPacket, vuAddress & 0xFFFF, VIF_CODE_UNPACK_V4_32, 1, 1);
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
    func_0012B9A0(xglCurrentPacket, VIF_CODE_FLUSH);
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

    xglMatrixStackLoad(params->stackMatrix);
    xglMatrixStackMul(matrix);
    xglMatrixStackSave(savedMatrix);
    xglPacketBeginUnpack(TIWATER_VU_TRANSFORMED);
    func_0012BCE8(xglCurrentPacket, savedMatrix, 4);
    xglPacketEndUnpack();
}

/* Sends the four quads of data at VU address 0x3C6. */
void tiWaterSendMatrix(const void *data) {
    xglPacketBeginUnpack(TIWATER_VU_MATRIX);
    func_0012BCE8(xglCurrentPacket, data, 4);
    xglPacketEndUnpack();
}

#include "quadword.h"

void xglMatrixFromAxes(Matrix4 matrix, const Vector4 *axis0, const Vector4 *axis1, const Vector4 *axis2);
/* The three axis rows that tiWaterSendOrientedMatrix passes to xglMatrixFromAxes. */
extern Vector4 tiWaterAxes[3];

/* Builds the matrix from the axes, copies data into a local matrix with its row 3 xyz cleared, and sends the stack product at VU address 0x3CA. */
void tiWaterSendOrientedMatrix(const void *data) {
    Matrix4 local;
    Matrix4 stack;

    xglMatrixFromAxes(stack, &tiWaterAxes[0], &tiWaterAxes[1], &tiWaterAxes[2]);
    QuadwordCopyMatrix(local, data);
    local[3][0] = local[3][1] = local[3][2] = 0.0f;
    xglMatrixStackLoad(stack);
    xglMatrixStackMul(local);
    xglMatrixStackSave(stack);
    xglPacketBeginUnpack(TIWATER_VU_ORIENTED);
    func_0012BCE8(xglCurrentPacket, stack, 4);
    xglPacketEndUnpack();
}

/* The four rows of the fixed matrix sent by tiWaterSendFixedMatrix. */
extern Matrix4 D_00631800;

/* Sends the matrix built from D_00631800 at VU address 0x3CE. */
void tiWaterSendFixedMatrix(void) {
    Matrix4 matrix;

    xglMatrixFromRows(matrix, D_00631800[0], D_00631800[1], D_00631800[2], D_00631800[3]);
    xglPacketBeginUnpack(TIWATER_VU_FIXED);
    func_0012BCE8(xglCurrentPacket, matrix, 4);
    xglPacketEndUnpack();
}

/* Sets the constants 1/512 and 7/16 in the block, then sends row80 at 0x3C0 and row70 at 0x3C1. */
void tiWaterSendParamRows(WaterFrameParams *params) {
    params->row80[3] = 0.001953125f;
    params->row70[3] = 0.4375f;
    xglPacketBeginUnpack(TIWATER_VU_ROW80);
    func_0012BCE8(xglCurrentPacket, params->row80, 1);
    xglPacketEndUnpack();
    xglPacketBeginUnpack(TIWATER_VU_ROW70);
    func_0012BCE8(xglCurrentPacket, params->row70, 1);
    xglPacketEndUnpack();
}

/* The item whose position (Vector4 at +0xD0) is sent to the packet. */
typedef struct WaterItem {
    u8 unmodeled_00[0xD0];
    Vector4 position;
} WaterItem;

/* The point (Vector4 at +0x30) that tiWaterSendDirection takes its direction towards. */
typedef struct WaterPointSource {
    u8 unmodeled_00[0x30];
    Vector4 point;
} WaterPointSource;

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

void tiWaterSendDirection(WaterItem *item, const WaterPointSource *source) {
    Vector4 dir;

    xglPacketBeginUnpack(TIWATER_VU_POSITION);
    func_0012BCE8(xglCurrentPacket, &item->position, 1);
    xglPacketEndUnpack();
    dir.x = source->point.x - item->position.x;
    dir.y = source->point.y - item->position.y;
    dir.z = source->point.z - item->position.z;
    dir.w = 1.0f;
    xglVectorNormal_001A9200(&dir, &dir);
    xglPacketBeginUnpack(TIWATER_VU_DIRECTION);
    func_0012BCE8(xglCurrentPacket, &dir, 1);
    xglPacketEndUnpack();
}

/* Sends the pair (x, y) at VU address 0x3DA. */
void tiWaterSendItemPair(f32 x, f32 y) {
    f32 xy[2];

    xy[0] = x;
    xy[1] = y;
    xglPacketBeginUnpack(0x3DA);
    func_0012BCE8(xglCurrentPacket, xy, 1);
    xglPacketEndUnpack();
}

/* Sends the pair (x, y) at VU address 0x3DE. */
void tiWaterSendFramePair(f32 x, f32 y) {
    f32 xy[2];

    xy[0] = x;
    xy[1] = y;
    xglPacketBeginUnpack(0x3DE);
    func_0012BCE8(xglCurrentPacket, xy, 1);
    xglPacketEndUnpack();
}

/* Sends one water frame: packet cache, reference table, transformed matrix, matrices, parameter rows, then the pair (0.7, 0.05). */
void tiWaterSendFrame(WaterFrameParams *params, const void *data) {
    xglPacketCaptureCurrent();
    xglPacketRefTable();
    tiWaterSendTransformed(params, data);
    tiWaterSendMatrix(data);
    tiWaterSendOrientedMatrix(data);
    tiWaterSendFixedMatrix();
    tiWaterSendParamRows(params);
    tiWaterSendFramePair(0.7f, 0.05f);
}

typedef struct WaterStripGrid {
    u8 unmodeled_00[0x08];
    int rowStride;
    int rowCount;
    u8 unmodeled_10[0x30];
    Vector4 origin;
    float cell;
    u8 unmodeled_54[0x28];
    u32 vertexOffset;
    u8 unmodeled_80[0x04];
    u32 vertexBase;
    u8 unmodeled_88[0x48];
    Matrix4 transform;
} WaterStripGrid;

/* Writes the strip of columns first..last between grid rows row and row + 1 into the scratchpad and sends it. */
void tiWaterSendStrip(int first, unsigned int last, int row, WaterStripGrid *grid) {
    u32 *gifTag = (u32 *)SCRATCH_GIF_TAG;
    DoublewordVector4 *quad = (DoublewordVector4 *)SCRATCH_QUAD_ROWS;
    unsigned int quadCount;
    DoublewordVector4 *vertexCopy;
    Vector4 *gridVertices;
    unsigned int rowStart;
    unsigned int gridColumn;
    float cellX;
    float cellZ;
    XglPacket *packet;

    quadCount = 2 * (last - first) + 2;
    gridVertices = (Vector4 *)(grid->vertexBase + grid->vertexOffset);
    *gifTag = (*gifTag & GIF_TAG_EOP) | (2 * (last - first) + 2);
    cellX = (float)first * grid->cell + grid->origin.x;
    cellZ = (float)row * -grid->cell + grid->origin.z;
    rowStart = row * grid->rowStride;
    packet = xglCurrentPacket;
    vertexCopy = quad + quadCount;
    for (gridColumn = first; gridColumn <= last; gridColumn++) {
        quad->vector.x = cellX;
        quad->vector.y = 0.0f;
        quad->vector.z = cellZ;
        quad->vector.w = 1.0f;
        quad++;
        vertexCopy->vector.x = gridVertices[rowStart + gridColumn].x;
        vertexCopy->vector.y = gridVertices[rowStart + gridColumn].y;
        vertexCopy->vector.z = gridVertices[rowStart + gridColumn].z;
        vertexCopy++;
        quad->vector.x = cellX;
        quad->vector.y = 0.0f;
        quad->vector.z = cellZ - grid->cell;
        quad->vector.w = 1.0f;
        quad++;
        vertexCopy->vector.x = gridVertices[rowStart + grid->rowStride + gridColumn].x;
        vertexCopy->vector.y = gridVertices[rowStart + grid->rowStride + gridColumn].y;
        vertexCopy->vector.z = gridVertices[rowStart + grid->rowStride + gridColumn].z;
        vertexCopy++;
        cellX += grid->cell;
    }
    func_0012B6E0(packet, 0x8000, VIF_CODE_UNPACK_V4_32, 1, 1);
    func_0012BCE8(packet, gifTag, 2 * quadCount + 1);
    func_0012B770(packet);
}

/* Strip count and column step of the grid (the strip width in columns). */
extern int tiWaterStripColumns;
/* Two adjacent vectors: the transform source (index 0) and its destination (index 1). */
extern Vector4 tiWatSimSourcePos[2];
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);

/* Sends the grid: copies the block into the GIF tag, sends the transformed origin, then one strip per row pair and column group. */
void tiWaterSendGrid(WaterStripGrid *grid, const void *block) {
    unsigned int strips;
    unsigned int row;
    unsigned int strip;
    int first;
    int column;
    XglPacket *packet;

    strips = (grid->rowStride - 1) / tiWaterStripColumns;
    packet = xglCurrentPacket;
    if (grid->rowStride >= 2 && grid->rowCount >= 2) {
        __builtin_memcpy((void *)SCRATCH_GIF_TAG, block, 16);
        xglVectorMulMat(&tiWatSimSourcePos[1], grid->transform, &tiWatSimSourcePos[0]);
        for (row = 0; row < grid->rowCount - 1; row++) {
            sceVif1PkCnt(packet, 0);
            first = 0;
            column = 0;
            for (strip = 0; strip < strips; strip++) {
                column += tiWaterStripColumns;
                tiWaterSendStrip(first, column, row, grid);
                first = column;
                func_0012B9A0(packet, VIF_CODE_MSCNT);
            }
            column = column + (grid->rowStride - tiWaterStripColumns * strips) - 1;
            if (first < column) {
                tiWaterSendStrip(first, column, row, grid);
                func_0012B9A0(packet, VIF_CODE_MSCNT);
                xglPacketFlush();
            }
        }
    }
}
