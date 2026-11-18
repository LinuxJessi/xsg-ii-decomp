#include "common.h"
#include "gs.h"
#include "main/tu026.h"
#include "main/TiDrawJoint.h"

/* Scratchpad packet addresses. */
#define SCRATCH_GIF_TAG ((TiDrawJointGifTag *)0x70000000)
#define SCRATCH_QUAD_ROWS ((unsigned int *)0x70000010)

/* Four-word GIF tag quadword written to the scratchpad packet. */
typedef struct TiDrawJointGifTag {
    unsigned int nloop_eop;       /* NLOOP (bits 0-14) and EOP (bit 15) */
    unsigned int nreg_flg_prim;   /* NREG (28-31), FLG (26-27), PRE (14) and PRIM (15-25) */
    unsigned int regs_lo;         /* register list, one nibble per register */
    unsigned int regs_hi;
} TiDrawJointGifTag;

/* Per-joint GIF tag state: each field is one bit group of the tag. */
typedef struct TiDrawJointGifState {
    unsigned char eop;            /* +0x00 */
    unsigned char unmodeled_01[0x3];
    int nloop;                    /* +0x04 */
    unsigned char nreg;           /* +0x08 */
    unsigned char flg;            /* +0x09 */
    unsigned char pre;            /* +0x0A */
    unsigned char prim_type;      /* +0x0B */
    unsigned char prim_iip;       /* +0x0C */
    unsigned char prim_tme;       /* +0x0D */
    unsigned char prim_fge;       /* +0x0E */
    unsigned char prim_abe;       /* +0x0F */
    unsigned char prim_aa1;       /* +0x10 */
    unsigned char prim_ctxt;      /* +0x11 */
    unsigned char prim_fix;       /* +0x12 */
} TiDrawJointGifState;

/* Writes the GIF tag header for state into the packet: REGS is RGBAQ, ST, XYZ2. */
void tiDrawJointSetGifTag(TiDrawJointGifTag *tag, const TiDrawJointGifState *state)
{
    unsigned int nreg_prim;
    unsigned int pre_flg;

    tag->nloop_eop = state->eop != 0 ? state->nloop | GIF_TAG_EOP : state->nloop;
    nreg_prim = (state->nreg << 28)
        | ((state->prim_type | (state->prim_iip << 3) | (state->prim_tme << 4) | (state->prim_fge << 5)
           | (state->prim_abe << 6) | (state->prim_aa1 << 7) | (state->prim_ctxt << 9) | (state->prim_fix << 10)) << 15);
    pre_flg = (state->pre << 14) | (state->flg << 26);
    tag->regs_lo = GIF_REGS_RGBAQ_ST_XYZ2;
    tag->regs_hi = 0;
    tag->nreg_flg_prim = nreg_prim | pre_flg;
}

#include "main/sdv_033.h"

void xglMatrixStackPush(void);
void xglMatrixStackLoad(float matrix[4][4]);
void xglMatrixStackPop(u32 value);
void xglMatrixStackRTPS(Vector4 *destination, const Vector4 *point, Vector4 *screenScale, Vector4 *screenOffset);
XglCamera *xglStudioGetActiveCamera(void);

/* Transforms count points through the active camera's view matrix and screen (RTPS). */
void tiDrawJointScreenPoints(unsigned int count, const Vector4 *source, Vector4 *destination)
{
    XglCamera *camera;
    const Vector4 *points;

    points = source;
    xglMatrixStackPush();
    camera = xglStudioGetActiveCamera();
    xglMatrixStackLoad(camera->volume[0].viewMatrix);
    while (count != 0) {
        xglMatrixStackRTPS(destination, points, &camera->screen.screenScale, &camera->screen.screenOffset);
        points += 1;
        destination += 1;
        count -= 1;
    }
    xglMatrixStackPop(1);
}

#include "shared.h"

void xglVectorMulMat(Vector4 *destination, float matrix[4][4], const Vector4 *vector);

/* Transforms count points by matrix into destination; each result gets w = 1. */
void tiDrawJointTransformPoints(unsigned int count, float matrix[4][4], const Vector4 *source,
                   Vector4 *destination)
{
    unsigned int index;

    index = 0;
    if (count != 0) {
        do {
            xglVectorMulMat(destination, matrix, source);
            destination->w = 1.0f;
            index += 1;
            source += 1;
            destination += 1;
        } while (index < count);
    }
}

extern TiDrawJointGifState tiDrawJointGifState;
const char tiDrawJointAssertSrc[8] = "pSrc";
const char tiDrawJointFileName[24] = "../TiDrawJoint.euc.c";
const char tiDrawJointAssertCol[8] = "pCol";
const char tiDrawJointAssertVif1Pk[8] = "pVIF1PK";
void sceVif1PkCnt(void *packet, unsigned int count);
void sceVif1PkOpenDirectHLCode(void *packet, unsigned int count);
void sceVif1PkAddDirectDataN(void *packet, void *data, unsigned int qwc);
void sceVif1PkCloseDirectHLCode(void *packet);
void *xglPacketGetCurrent(void);
void assertFailed(const char *message, const char *file, int line);

/* Packs count points, two per pair, into the VIF1 direct packet: each vertex is an RGBAQ quadword
 * (colour words plus alpha 255), an ST quadword of zeros and an XYZ2 quadword. */
void tiDrawJointPackPoints(unsigned int count, const Vector4 *source, const unsigned int *colour, int flag)
{
    const int *words;
    unsigned int *cursor;
    unsigned int vertices;
    unsigned int index;
    tiDrawJointGifState.prim_tme = 0;
    vertices = 0;
    cursor = SCRATCH_QUAD_ROWS;
    if (source == 0) {
        assertFailed(tiDrawJointAssertSrc, tiDrawJointFileName, 90);
    }
    if (colour == 0) {
        assertFailed(tiDrawJointAssertCol, tiDrawJointFileName, 91);
    }
    index = 0;
    if (count != 0) {
        words = (const int *)source;
        do {
            if ((words[0] & 0xffff0000) != 0 || (words[1] & 0xffff0000) != 0 || words[2] < 0
                || (words[4] & 0xffff0000) != 0 || (words[5] & 0xffff0000) != 0) {
                return;
            }
            if (words[6] < 0) {
                return;
            }
            vertices += 2;
            cursor[0] = colour[0];
            cursor[1] = colour[1];
            cursor[2] = colour[2];
            cursor[3] = 255;
            cursor += 4;
            cursor[0] = 0;
            cursor[1] = 0;
            cursor[2] = 0;
            cursor[3] = 0;
            cursor += 4;
            cursor[0] = words[0];
            cursor[1] = words[1];
            if (flag != 0) {
                cursor[2] = words[2];
            } else {
                cursor[2] = 0xffff00;
            }
            cursor[3] = 0;
            cursor += 4;
            cursor[0] = colour[0];
            cursor[1] = colour[1];
            cursor[2] = colour[2];
            cursor[3] = 255;
            cursor += 4;
            cursor[0] = 0;
            cursor[1] = 0;
            cursor[2] = 0;
            cursor[3] = 0;
            cursor += 4;
            cursor[0] = words[4];
            cursor[1] = words[5];
            if (flag != 0) {
                cursor[2] = words[2];
            } else {
                cursor[2] = 0xffff00;
            }
            cursor[3] = 0;
            cursor += 4;
            index += 2;
            words += 8;
        } while (index < count);
    }
    if (vertices != 0) {
        cursor = xglPacketGetCurrent();
        if (cursor == 0) {
            assertFailed(tiDrawJointAssertVif1Pk, tiDrawJointFileName, 150);
        }
        tiDrawJointGifState.nloop = vertices;
        tiDrawJointSetGifTag(SCRATCH_GIF_TAG, &tiDrawJointGifState);
        sceVif1PkCnt(cursor, 0);
        sceVif1PkOpenDirectHLCode(cursor, 0);
        sceVif1PkAddDirectDataN(cursor, SCRATCH_GIF_TAG, vertices * 3 + 1);
        sceVif1PkCloseDirectHLCode(cursor);
    }
}

const char tiDrawJointAssertTransMat[16] = "pTransMat";
extern AlignedVector4 tiDrawJointAxisPointsX[];
extern Vector4 tiDrawJointAxisPointsY[];
extern Vector4 tiDrawJointAxisPointsZ[];
extern unsigned int tiDrawJointAxisColourX[];
extern unsigned int tiDrawJointAxisColourY[];
extern unsigned int tiDrawJointAxisColourZ[];

/* Draws the three point stages: each transforms two points through matrix, screens them and
 * packs them with its colour block. value is written into one point of each stage first. */
void tiDrawJointDrawPoints(float matrix[4][4], float value)
{
    Vector4 screen[2];
    Vector4 local[2];

    if (matrix == 0) {
        assertFailed(tiDrawJointAssertTransMat, tiDrawJointFileName, 185);
    }
    tiDrawJointAxisPointsX[1].x = value;
    tiDrawJointAxisPointsY[1].y = value;
    tiDrawJointAxisPointsZ[1].z = value;
    tiDrawJointTransformPoints(2, matrix, tiDrawJointAxisPointsX, local);
    tiDrawJointScreenPoints(2, local, screen);
    tiDrawJointPackPoints(2, screen, tiDrawJointAxisColourX, 1);
    tiDrawJointTransformPoints(2, matrix, tiDrawJointAxisPointsY, local);
    tiDrawJointScreenPoints(2, local, screen);
    tiDrawJointPackPoints(2, screen, tiDrawJointAxisColourY, 1);
    tiDrawJointTransformPoints(2, matrix, tiDrawJointAxisPointsZ, local);
    tiDrawJointScreenPoints(2, local, screen);
    tiDrawJointPackPoints(2, screen, tiDrawJointAxisColourZ, 1);
}

const char D_0068F788[16] = "pCurrMat";
static Vector4 tiDrawJointMeshPoints[16];
static Vector4 tiDrawJointMeshTransformed[16];
static Vector4 D_00989C50[16];

/* Draws the 16-point mesh tiDrawJointMeshPoints through current_matrix (asserted as pCurrMat).
 * colour reaches the packet builder in its colour position (asserted as pCol there). */
void tiDrawJointDrawMesh(float current_matrix[4][4], const unsigned int *colour)
{
    if (current_matrix == 0) {
        assertFailed(D_0068F788, tiDrawJointFileName, 210);
    }
    tiDrawJointTransformPoints(16, current_matrix, tiDrawJointMeshPoints, tiDrawJointMeshTransformed);
    tiDrawJointScreenPoints(16, tiDrawJointMeshTransformed, D_00989C50);
    tiDrawJointPackPoints(16, D_00989C50, colour, 0);
}

extern Vector4 tiDrawJointMeshBase[];
extern Vector4 tiDrawJointRefAxis[];
float sqrtf(float value);
int XrgQuaternionFromVectors(Vector4 *destination, const Vector4 *table, const Vector4 *axis);
void XrgQuaternionToMatrix(float matrix[4][4], const Vector4 *quaternion);

/* Normalises the axis into a Vector4 with w = 1, records its length in four table points, then
 * transforms the table: negated when XrgQuaternionFromVectors returns 0, else through the quaternion matrix. */
void tiDrawJointOrientMesh(const float *axis)
{
    Vector4 normal;
    Vector4 quaternion;
    float matrix[4][4];
    float length;
    float scale;
    unsigned int index;

    length = __builtin_sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    if (length != 1.0f) {
        if (length == 0.0f) {
            length = 1.0e-7f;
        }
        scale = 1.0f / length;
        normal.x = axis[0] * scale;
        normal.y = axis[1] * scale;
        normal.z = axis[2] * scale;
    } else {
        normal.x = axis[0];
        normal.y = axis[1];
        normal.z = axis[2];
    }
    normal.w = 1.0f;
    tiDrawJointMeshBase[9].x = length;
    tiDrawJointMeshBase[11].x = length;
    tiDrawJointMeshBase[13].x = length;
    tiDrawJointMeshBase[15].x = length;
    if (XrgQuaternionFromVectors(&quaternion, tiDrawJointRefAxis, &normal) == 0) {
        for (index = 0; index < 16; index += 1) {
            tiDrawJointMeshPoints[index].x = -tiDrawJointMeshBase[index].x;
            tiDrawJointMeshPoints[index].y = -tiDrawJointMeshBase[index].y;
            tiDrawJointMeshPoints[index].z = -tiDrawJointMeshBase[index].z;
            tiDrawJointMeshPoints[index].w = 1.0f;
        }
    } else {
        XrgQuaternionToMatrix(matrix, &quaternion);
        for (index = 0; index < 16; index += 1) {
            xglVectorMulMat(&tiDrawJointMeshPoints[index], matrix, &tiDrawJointMeshBase[index]);
        }
    }
}

#include "quadword.h"

extern unsigned int tiDrawJointColourDefault[];
extern unsigned int tiDrawJointColourSelectA[];
extern unsigned int tiDrawJointColourDefaultE[];
extern unsigned int tiDrawJointColourSelectE[];
const char tiDrawJointAssertGlobalMat[16] = "pGlobalMat";

typedef struct TiDrawJointEntry64 {
    int matrixIndex;
    unsigned char unmodeled_04[0x3C];
} TiDrawJointEntry64;

typedef struct TiDrawJointEntry80 {
    int matrixIndex;
    float axis[3];
    unsigned char unmodeled_10[0x40];
} TiDrawJointEntry80;

typedef struct TiDrawJointEntry112 {
    int matrixIndex;
    unsigned char unmodeled_04[0x08];
    float axis[3];
    unsigned char unmodeled_18[0x58];
} TiDrawJointEntry112;

/* 176-byte header, then 16 bytes per counted vertex. */
typedef struct TiDrawJointPart {
    int matrixIndex;
    unsigned char unmodeled_04[0x18];
    unsigned int counts[3];
    unsigned char unmodeled_28[0x08];
    float axis[3];
    unsigned char unmodeled_3c[0x74];
} TiDrawJointPart;

/* Six counted lists; the offsets address the list tails. */
typedef struct TiDrawJointList {
    unsigned char unmodeled_00[0x10];
    unsigned int count0;
    unsigned int count1;
    unsigned int count2;
    unsigned int count3;
    unsigned int count4;
    unsigned int count5;
    unsigned int offset1;
    unsigned int offset2;
    unsigned int offset3;
    unsigned int offset4;
    unsigned int offset5;
    unsigned char unmodeled_3c[0x34];
    TiDrawJointEntry64 head[2];
} TiDrawJointList;

typedef struct TiDrawJointState {
    unsigned char unmodeled_0000[0x710];
    TiDrawJointList *list;
    unsigned char unmodeled_0714[0x8C];
    short selectMesh1;
    short selectMesh2;
} TiDrawJointState;

typedef struct TiDrawJointGlobals {
    unsigned char unmodeled_0000[0xF40];
    float pointExtent;
    float pointValue;
} TiDrawJointGlobals;

extern TiDrawJointGlobals tiDrawJointGlobals;
void xglMatrixMul(void *destination, void *left, void *right);
void xglMatrixStackUnit(void);

/* Builds the point table from the extent, then draws the six list groups through the current matrix. */
void tiDrawJointDrawLists(TiDrawJointState *state, float (*pGlobalMat)[4][4], float (*current)[4][4])
{
    float jointMatrix[4][4];
    int meshCount;
    unsigned int drawCount;
    unsigned int i;
    unsigned int count5;
    unsigned int stride;
    char *nextPart;
    TiDrawJointEntry80 *partsA;
    TiDrawJointEntry80 *partsB;
    TiDrawJointEntry80 *partsC;
    TiDrawJointEntry112 *partsD;
    TiDrawJointPart *partsE;
    TiDrawJointEntry64 *partsF;
    TiDrawJointEntry64 *head;

    partsA = (TiDrawJointEntry80 *)((char *)state->list + 240);
    partsB = (TiDrawJointEntry80 *)((char *)state->list + state->list->offset1);
    partsC = (TiDrawJointEntry80 *)((char *)state->list + state->list->offset2);
    partsD = (TiDrawJointEntry112 *)((char *)state->list + state->list->offset3);
    partsE = (TiDrawJointPart *)((char *)state->list + state->list->offset4);
    partsF = (TiDrawJointEntry64 *)((char *)state->list + state->list->offset5);
    head = state->list->head;
    meshCount = 0;
    if (pGlobalMat == 0) {
        assertFailed(tiDrawJointAssertGlobalMat, tiDrawJointFileName, 309);
    }
    tiDrawJointMeshBase[1].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[1].y = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[3].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[3].y = -tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[5].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[5].z = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[7].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[7].z = -tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[8].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[8].y = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[10].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[10].y = -tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[14].z = -tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[12].x = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[12].z = tiDrawJointGlobals.pointExtent;
    tiDrawJointMeshBase[14].x = tiDrawJointGlobals.pointExtent;
    xglMatrixStackUnit();

    drawCount = 0;
    {
        i = 0;
        do {
            if (head[i].matrixIndex >= 0) {
                xglMatrixMul(jointMatrix, current, &pGlobalMat[head[i].matrixIndex]);
            } else {
                QuadwordCopyMatrix(jointMatrix, current);
            }
            if (tiDrawJointGlobals.pointValue != 0.0f) {
                xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
            }
            i += 1;
        } while (i < 2);
    }

    {
        for (i = 0; i < state->list->count0 - 2; i += 1) {
            if (partsA[i].matrixIndex >= 0) {
                tiDrawJointOrientMesh(partsA[i].axis);
                xglMatrixMul(jointMatrix, current, &pGlobalMat[partsA[i].matrixIndex]);
                if ((int)i - 2 == state->selectMesh1) {
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourSelectA);
                } else {
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourDefault);
                }
                if (tiDrawJointGlobals.pointValue != 0.0f) {
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                        tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                }
            }
        }
    }

    if (state->list->offset1 != 0) {
        i = 0;
        if (state->list->count1 != 0) {
            do {
                if (partsB[i].matrixIndex >= 0) {
                    tiDrawJointOrientMesh(partsB[i].axis);
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[partsB[i].matrixIndex]);
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourDefault);
                    if (tiDrawJointGlobals.pointValue != 0.0f) {
                        xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                        tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                    }
                }
                i += 1;
            } while (i < state->list->count1);
        }
    }

    if (state->list->offset2 != 0) {
        i = 0;
        if (state->list->count2 != 0) {
            do {
                if (partsC[i].matrixIndex >= 0) {
                    tiDrawJointOrientMesh(partsC[i].axis);
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[partsC[i].matrixIndex]);
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourDefault);
                    if (tiDrawJointGlobals.pointValue != 0.0f) {
                        xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                        tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                    }
                }
                i += 1;
            } while (i < state->list->count2);
        }
    }

    if (state->list->offset3 != 0) {
        i = 0;
        if (state->list->count3 != 0) {
            do {
                if (partsD[i].matrixIndex >= 0) {
                    tiDrawJointOrientMesh(partsD[i].axis);
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[partsD[i].matrixIndex]);
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourDefault);
                    if (tiDrawJointGlobals.pointValue != 0.0f) {
                        xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                        tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                    }
                }
                i += 1;
            } while (i < state->list->count3);
        }
    }

    i = 0;
    if (state->list->count4 != 0) {
        do {
            stride = (partsE->counts[0] + partsE->counts[1] + partsE->counts[2]) << 4;
            if (partsE->matrixIndex >= 0) {
                tiDrawJointOrientMesh(partsE->axis);
                xglMatrixMul(jointMatrix, current, &pGlobalMat[partsE->matrixIndex]);
                if (meshCount == state->selectMesh2) {
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourSelectE);
                } else {
                    tiDrawJointDrawMesh(jointMatrix, tiDrawJointColourDefaultE);
                }
                if (tiDrawJointGlobals.pointValue != 0.0f) {
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                        tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                }
            }
            nextPart = (char *)partsE + 176;
            partsE = (TiDrawJointPart *)(nextPart + stride);
            meshCount++;
            i += 1;
        } while (i < state->list->count4);
    }

    if (state->list->offset5 != 0) {
        i = 0;
        if (state->list->count5 != 0) {
            do {
                if (partsF[i].matrixIndex >= 0) {
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[partsF[i].matrixIndex]);
                } else {
                    QuadwordCopyMatrix(jointMatrix, current);
                }
                if (tiDrawJointGlobals.pointValue != 0.0f) {
                    xglMatrixMul(jointMatrix, current, &pGlobalMat[drawCount++]);
                    tiDrawJointDrawPoints(jointMatrix, tiDrawJointGlobals.pointValue);
                }
                i += 1;
            } while (i < state->list->count5);
        }
    }

    state->selectMesh1 = -1;
    state->selectMesh2 = -1;
}
/* Per-joint GIF tag state; the struct is 19 bytes, the rest of its 32-byte slot is alignment fill. */
TiDrawJointGifState tiDrawJointGifState = { 1, { 0, 0, 0 }, 0, 3, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0 };

/* Axis end points (index 1 is moved to the axis length) and their colours. */
AlignedVector4 tiDrawJointAxisPointsX[2] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } };
unsigned int tiDrawJointAxisColourX[4] = { 0xffu, 0u, 0u, 0xffu };
Vector4 tiDrawJointAxisPointsY[2] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } };
unsigned int tiDrawJointAxisColourY[4] = { 0u, 0xffu, 0u, 0xffu };
Vector4 tiDrawJointAxisPointsZ[2] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } };
unsigned int tiDrawJointAxisColourZ[4] = { 0u, 0u, 0xffu, 0xffu };

/* Base of the 16-point mesh, before orientation. */
Vector4 tiDrawJointMeshBase[16] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, -0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, -0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, -0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, -0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

Vector4 tiDrawJointRefAxis[1] = { { 1.0f, 0.0f, 0.0f, 1.0f } };

unsigned int tiDrawJointColourDefault[4] = { 0x60u, 0x60u, 0xaau, 0xffu };
unsigned int tiDrawJointColourSelectA[4] = { 0xccu, 0xaau, 0xffu, 0xffu };
unsigned int tiDrawJointColourDefaultE[4] = { 0xaau, 0x60u, 0x60u, 0xffu };
unsigned int tiDrawJointColourSelectE[4] = { 0xffu, 0xccu, 0xaau, 0xffu };
