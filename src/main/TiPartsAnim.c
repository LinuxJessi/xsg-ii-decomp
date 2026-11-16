#include "common.h"
#include "shared.h"
#include "main/tu026.h"
#include "main/tu254.h"
#include "main/tu256.h"
#include "main/TiXacAnimCurve.h"

void xglCurMatrixGet(Matrix4 matrix);
void xglCurMatrixSet(Matrix4 matrix);
extern unsigned char tiPartsAnimAssertExpr[];
extern unsigned char tiPartsAnimSourceFile[];

/* The pack read by tiPartsAnimSamplePose: curveParam is the curve parameter of every joint,
 * pPamData the start of the animation data. */
typedef struct TiPartsPack {
    unsigned char unmodeled_00[0x18];
    float curveParam;
    unsigned char unmodeled_1C[0xB8];
    u8 *pPamData;
} TiPartsPack;

/* Samples `count` joints into matrices, reading each joint's curves from pPack->pPamData. The nine curve values
 * of a joint are: offset (0-2), rotation X, Y, Z (3-5) and pivot (6-8). */
void tiPartsAnimSamplePose(Matrix4 *matrices, TiPartsPack *pPack, int count)
{
    float values[9];
    float negPivot[3];
    u8 *cursor;
    u8 **ppJointHeader;
    int i;

    if (pPack->pPamData == 0) {
        assertFailed(tiPartsAnimAssertExpr, tiPartsAnimSourceFile, 58);
    }
    cursor = pPack->pPamData + 12;
    for (i = 0; i < count; i++) {
        ppJointHeader = &cursor;
        xacReadJointCurves9(values, ppJointHeader, pPack->curveParam);
        tiVu0LoadFixedMatrix();
        if (values[0] != 0.0f || values[1] != 0.0f || values[2] != 0.0f) {
            xglCurMulTranslate(values);
        }
        if (values[6] != 0.0f || values[7] != 0.0f || values[8] != 0.0f) {
            negPivot[0] = -values[6];
            negPivot[1] = -values[7];
            negPivot[2] = -values[8];
            xglCurMulTranslate(&values[6]);
            if (values[5] != 0.0f) {
                xglCurMulRotateZ(values[5]);
            }
            if (values[4] != 0.0f) {
                xglCurMulRotateY(values[4]);
            }
            if (values[3] != 0.0f) {
                xglCurMulRotateX(values[3]);
            }
            xglCurMulTranslate(negPivot);
        } else {
            if (values[5] != 0.0f) {
                xglCurMulRotateZ(values[5]);
            }
            if (values[4] != 0.0f) {
                xglCurMulRotateY(values[4]);
            }
            if (values[3] != 0.0f) {
                xglCurMulRotateX(values[3]);
            }
        }
        xglCurMatrixGet(matrices[i]);
    }
}

/* Samples one joint: reads nine curve values into offset, rotation and pivot, then builds the matrix
 * (translate by offset, pivot around the rotation when the pivot is nonzero, rotations Z, Y, X). */
void tiPartsAnimSampleJoint(Matrix4 matrix, u8 **ppJointHeader, float curveParam)
{
    float values[9];
    float negPivot[3];

    xacReadJointCurves9(values, ppJointHeader, curveParam);
    tiVu0LoadFixedMatrix();
    if (values[0] != 0.0f || values[1] != 0.0f || values[2] != 0.0f) {
        xglCurMulTranslate(values);
    }
    if (values[6] != 0.0f || values[7] != 0.0f || values[8] != 0.0f) {
        negPivot[0] = -values[6];
        negPivot[1] = -values[7];
        negPivot[2] = -values[8];
        xglCurMulTranslate(&values[6]);
        if (values[5] != 0.0f) {
            xglCurMulRotateZ(values[5]);
        }
        if (values[4] != 0.0f) {
            xglCurMulRotateY(values[4]);
        }
        if (values[3] != 0.0f) {
            xglCurMulRotateX(values[3]);
        }
        xglCurMulTranslate(negPivot);
    } else {
        if (values[5] != 0.0f) {
            xglCurMulRotateZ(values[5]);
        }
        if (values[4] != 0.0f) {
            xglCurMulRotateY(values[4]);
        }
        if (values[3] != 0.0f) {
            xglCurMulRotateX(values[3]);
        }
    }
    xglCurMatrixGet(matrix);
}

/* One node of the old table format (0x1C bytes): its matrix index and transform. */
typedef struct TiPartsSlot {
    int matrixIndex;
    float offset[3];
    float rotation[3];
} TiPartsSlot;

/* One node of the new table format (0x20 bytes): the id it is searched by, then its matrix index and transform. */
typedef struct TiPartsKey {
    int id;
    int matrixIndex;
    float offset[3];
    float rotation[3];
} TiPartsKey;

/* The node table of an MRPack handle: threshold at +4 (below 1.1f the slots are indexed, else searched by id),
 * count at +8, entries from +0x10. */
typedef struct TiPartsTable {
    unsigned char unmodeled_00[4];
    float threshold;
    int count;
    unsigned char unmodeled_0C[4];
    TiPartsSlot slots[0];
} TiPartsTable;

/* The MRPack handle that tiMRPackageGetNodeMatrix reads (the actor's handle640): its node table at +0xD8, the matrix array
 * at +0xE0, the same offsets as the status block of TiDynamicsColl.c. */
typedef struct TiMRPackageHandle {
    unsigned char unmodeled_00[0xD8];
    TiPartsTable *table;
    unsigned char unmodeled_DC[4];
    Matrix4 *matrices;
} TiMRPackageHandle;

/* Writes the matrix of node `nodeKey` into outMatrix: the old format indexes the table directly, the newer one
 * searches it by id. useBaseMatrix selects the start matrix: the handle's matrix of the node (set) or the fixed one.
 * Returns 0 when the node is absent. */
int tiMRPackageGetNodeMatrix(TiMRPackageHandle *handle, int nodeKey, Matrix4 outMatrix, int useBaseMatrix)
{
    TiPartsTable *table;
    TiPartsSlot *slots;
    TiPartsKey *keys;
    TiPartsKey *key;
    unsigned int nodeIndex;

    table = handle->table;
    if (table == 0) {
        return 0;
    }
    if (table->threshold < 1.1f) {
        slots = table->slots;
        if ((unsigned int) nodeKey >= (unsigned int) table->count) {
            return 0;
        }
        nodeIndex = (unsigned int) nodeKey;
        if (useBaseMatrix != 0) {
            xglCurMatrixSet(handle->matrices[slots[nodeIndex].matrixIndex]);
        } else {
            tiVu0LoadFixedMatrix();
        }
        if (slots[nodeIndex].rotation[2] != 0.0f) {
            xglCurMulRotateZ(slots[nodeIndex].rotation[2]);
        }
        if (slots[nodeIndex].rotation[1] != 0.0f) {
            xglCurMulRotateY(slots[nodeIndex].rotation[1]);
        }
        if (slots[nodeIndex].rotation[0] != 0.0f) {
            xglCurMulRotateX(slots[nodeIndex].rotation[0]);
        }
        if (slots[nodeIndex].offset[0] != 0.0f || slots[nodeIndex].offset[1] != 0.0f || slots[nodeIndex].offset[2] != 0.0f) {
            xglCurMulTranslate(slots[nodeIndex].offset);
        }
        xglCurMatrixGet(outMatrix);
    } else {
        keys = (TiPartsKey *) table->slots;
        key = 0;
        for (nodeIndex = 0; nodeIndex < handle->table->count; nodeIndex++) {
            if (keys[nodeIndex].id == nodeKey) {
                key = &keys[nodeIndex];
                break;
            }
        }
        if (key == 0) {
            return 0;
        }
        if (useBaseMatrix != 0) {
            xglCurMatrixSet(handle->matrices[key->matrixIndex]);
        } else {
            tiVu0LoadFixedMatrix();
        }
        if (key->rotation[2] != 0.0f) {
            xglCurMulRotateZ(key->rotation[2]);
        }
        if (key->rotation[1] != 0.0f) {
            xglCurMulRotateY(key->rotation[1]);
        }
        if (key->rotation[0] != 0.0f) {
            xglCurMulRotateX(key->rotation[0]);
        }
        if (key->offset[0] != 0.0f || key->offset[1] != 0.0f || key->offset[2] != 0.0f) {
            xglCurMulTranslate(key->offset);
        }
        xglCurMatrixGet(outMatrix);
    }
    return 1;
}
