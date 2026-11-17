#include "common.h"
#include "shared.h"
#include "main/tu026.h"
#include "main/tu254.h"
#include "main/tu256.h"
#include "main/tu255.h"
#include "main/TiMRPackage.h"
/* TiMotionTarget.flags: the target applies its angle. */
#define TI_TARGET_FLAG_ANGLE 0x10
/* TiHrc* node flags: undo the parent's scale. */
#define TI_NODE_FLAG_INV_PARENT_SCALE 2
/* TiMotionPackage.flags: some node has a non-unit scale. */
#define TI_PACKAGE_FLAG_SCALED 0x20
/* Every hierarchy starts with two locators, then the joints. */
#define TI_LOCATOR_COUNT 2
/* TiHrcSkip.limitFlags: a minimum or maximum clamp on each rotation value (X, Y, Z = values[3..5]). */
#define TI_SKIP_LIMIT_ROT_X_MIN 0x1
#define TI_SKIP_LIMIT_ROT_X_MAX 0x2
#define TI_SKIP_LIMIT_ROT_Y_MIN 0x4
#define TI_SKIP_LIMIT_ROT_Y_MAX 0x8
#define TI_SKIP_LIMIT_ROT_Z_MIN 0x10
#define TI_SKIP_LIMIT_ROT_Z_MAX 0x20

void xglCurMatrixGet(Matrix4 matrix);
void xglCurSetRotateX(float value);
void xglMatrixMul(void *destination, void *left, void *right);
extern const unsigned char tiMotionAssertPPlaceMat[];
extern const unsigned char tiMotionSourceFile[];

typedef struct TiMotionTarget {
    int flags;
    unsigned char unmodeled_04[0x1C];
    float vector[4];
    float bounds[4];
    float rotateX;
    float angle;
    float rotateZ;
} TiMotionTarget;

/* Multiplies `destination` by a rotation of
 * -angle / 2 when the target's angle flag is set and the range is collapsed (range[1] <= range[0]). */
void tiMotionApplyTargetAngle(Matrix4 destination, void *pPlaceMat, TiMotionTarget *target, const float *range)
{
    Matrix4 delta;

    if (pPlaceMat == 0) {
        assertFailed(tiMotionAssertPPlaceMat, tiMotionSourceFile, 14);
    }
    if ((target->flags & TI_TARGET_FLAG_ANGLE) == 0 || range[1] > range[0]) {
        return;
    }
    xglCurSetRotateX(-target->angle * 0.5f);
    xglCurMatrixGet(delta);
    xglMatrixMul(destination, destination, delta);
}

void xglMatrixInverse(void *destination, void *source);
void xglVectorMulMat(void *destination, void *matrix, void *vector);
float xglAtan2Angle(float y, float x);
void xglCurSetRotateY(float value);
void xglCurMulRotateX(float value);
void xglCurMulRotateZ(float value);

void tiMotionAimTarget(Matrix4 destination, void *pPlaceMat, TiMotionTarget *target, const float *range)
{
    Matrix4 placeInverse;
    Matrix4 placeMatrix;
    float local[4];
    float angle;
    float value;

    if (pPlaceMat == 0) {
        assertFailed(tiMotionAssertPPlaceMat, tiMotionSourceFile, 36);
    }
    if ((target->flags & TI_TARGET_FLAG_ANGLE) != 0) {
        xglMatrixMul(placeMatrix, pPlaceMat, destination);
        xglMatrixInverse(placeInverse, placeMatrix);
        xglVectorMulMat(local, placeInverse, target->vector);
        if (range[1] > range[0]) {
            float twoPi;
            twoPi = 6.2831855f;
            target->angle = 1.5707964f - xglAtan2Angle(local[0], local[2]) + twoPi;
            target->rotateX = 6.2831855f - xglAtan2Angle(local[2], local[1]);
            target->rotateZ = 0.0f;
            while (3.1415927f < target->angle) {
                target->angle = target->angle - 6.2831855f;
            }
            if (3.1415927f < target->rotateX) {
                angle = target->rotateX;
                do {
                    value = angle - 6.2831855f;
                    angle = value;
                } while (3.1415927f < value);
                target->rotateX = value;
            }
            if (target->angle < -0.9f) {
                target->angle = -0.9f;
            }
            if (0.9f < target->angle) {
                target->angle = 0.9f;
            }
            if (target->rotateX < -0.4f) {
                target->rotateX = -0.4f;
            }
            if (0.4f < target->rotateX) {
                target->rotateX = 0.4f;
            }
            target->angle = (target->angle - target->bounds[1]) * 0.15f + target->bounds[1];
            value = (target->rotateX - target->bounds[0]) * 0.15f + target->bounds[0];
            target->rotateX = value;
        } else {
            target->rotateZ = xglAtan2Angle(local[0], local[1]);
            target->angle = -xglAtan2Angle(local[0], local[2]) + 6.2831855f;
            target->rotateX = 0.0f;
            while (3.1415927f < target->rotateZ) {
                target->rotateZ = target->rotateZ - 6.2831855f;
            }
            while (3.1415927f < target->angle) {
                target->angle = target->angle - 6.2831855f;
            }
            if (target->rotateZ < -0.4f) {
                target->rotateZ = -0.4f;
            }
            if (0.4f < target->rotateZ) {
                target->rotateZ = 0.4f;
            }
            if (target->angle < -0.9f) {
                target->angle = -0.9f;
            }
            if (0.9f < target->angle) {
                target->angle = 0.9f;
            }
            target->angle = (target->angle - target->bounds[1]) * 0.15f + target->bounds[1];
            target->rotateZ = (target->rotateZ - target->bounds[2]) * 0.15f + target->bounds[2];
        }
    } else {
        value = target->rotateX + target->angle + target->rotateZ;
        if (value == 0.0f) {
            return;
        }
        target->rotateX = target->rotateX * 0.8f;
        target->angle = target->angle * 0.8f;
        target->rotateZ = target->rotateZ * 0.8f;
    }
    xglCurSetRotateY(target->angle);
    if (range[1] > range[0]) {
        xglCurMulRotateX(target->rotateX);
    } else {
        xglCurMulRotateZ(target->rotateZ);
    }
    xglCurMatrixGet(placeMatrix);
    xglMatrixMul(destination, destination, placeMatrix);
}

typedef struct TiMotionTransform {
    int parent;
    float offset[3];
    float rotation[3];
    float scale[3];
} TiMotionTransform;

void xglCurMulScaleXYZ(void *vector);

/* One locator of the hierarchy (0x40 bytes). */
typedef struct TiHrcLocator {
    int parent;
    unsigned char unmodeled_04[0x24];
    float scale[3];
    int flags;
    unsigned char unmodeled_38[0x8];
} TiHrcLocator;

/* One joint of the hierarchy (0x50 bytes). */
typedef struct TiHrcJoint {
    int parent;
    float range[3];
    float orient[3];
    unsigned char unmodeled_1C[0x18];
    float scale[3];
    float rotationScale;
    int flags;
    unsigned char unmodeled_48[0x8];
} TiHrcJoint;

/* One dynamics node: 0xB0 bytes followed by vectorCount[0..2] vectors. */
typedef struct TiHrcDynamics {
    int parent;
    unsigned char unmodeled_04[0x18];
    int vectorCount[3];
    int flags;
    float weight;
    unsigned char unmodeled_30[0x10];
    float orient[3];
    unsigned char unmodeled_4C[0x24];
    float scale[3];
    unsigned char unmodeled_7C[0x34];
    float vectors[0][4];
} TiHrcDynamics;

/* One static node (0x40 bytes): a fixed transform. */
typedef struct TiHrcStatic {
    TiMotionTransform transform;
    unsigned char unmodeled_28[0x18];
} TiHrcStatic;

typedef struct TiHrcData {
    unsigned char unmodeled_00[0x10];
    int jointCount;
    int joint2Count;
    int joint3Count;
    int skipCount;
    int dynamicsCount;
    int staticCount;
    int joint2Offset;
    int joint3Offset;
    int skipOffset;
    int dynamicsOffset;
    int staticOffset;
    unsigned char unmodeled_3C[0x34];
    TiHrcLocator locators[2];
    TiHrcJoint joints[1];
} TiHrcData;

typedef struct TiXacData {
    int unmodeled_00;
    float version;
    unsigned char unmodeled_08[0x8];
    int jointCount;
    int joint2Count;
    int unmodeled_18;
    int dynamicsCount;
    int jointOffset;
    int joint2Offset;
    int unmodeled_28;
    int dynamicsOffset;
} TiXacData;

typedef union TiDynamicsOut {
    Matrix4 matrix;
} TiDynamicsOut;

/* local view: the solver's package view. Its pLocalMat/pWorldMat (Matrix4 *), pHrcData (TiHrcData *), pDynMat and
 * pPlaceMat types differ from the shared TiMRPackage in include/main/TiMRPackage.h, and the code changes with them. */
typedef struct TiMotionPackage {
    unsigned char unmodeled_00[0x4];
    int targetNode;
    unsigned char unmodeled_08[0x8];
    int flags;
    int unmodeled_14;
    float frame;
    unsigned char unmodeled_1C[0x44];
    int skipFlags;
    unsigned char unmodeled_64[0x4];
    float skipFrame;
    unsigned char unmodeled_6C[0x44];
    float scale[3];
    unsigned char unmodeled_BC[0x4];
    void *pAnimData; /* TiXacData, or TiXacData2 for the second format */
    unsigned char unmodeled_C4[0x4];
    struct TiXacData2 *pSkipAnimData;
    unsigned char unmodeled_CC[0x4];
    TiHrcData *pHrcData;
    unsigned char unmodeled_D4[0x8];
    Matrix4 *pLocalMat;
    Matrix4 *pWorldMat;
    unsigned char unmodeled_E4[0x8];
    TiDynamicsOut *pDynMat;
    /* The place matrix the target solver reads (pPlaceMat). */
    Matrix4 *pPlaceMat;
} TiMotionPackage;

extern const unsigned char tiMotionAssertPLocalMat[];
extern const unsigned char tiMotionAssertPHrcData[];
void xacEvalTransLocator(float *values, TiHrcLocator *locator, unsigned char **cursor, float frame);
void xacEvalTransJoint(float *values, TiHrcJoint *joint, unsigned char **cursor, float frame);
void xacEvalTransDynamics(float *values, TiHrcDynamics *dynamics, unsigned char **cursor, float frame);
void xacEvalTransDynamicsNoWeight(float *values, TiHrcDynamics *dynamics, unsigned char **cursor, float frame);
float *tiMotionFindParentScale(TiMotionPackage *pMRPack, int index);
void xglCurMulInvScaleXYZ(float *scale);

/* One skip (face) node of the table at hrc->skipOffset (0x70 bytes): a rotation evaluated against per-axis limits. */
typedef struct TiHrcSkip {
    int parent;
    unsigned int limitFlags;
    unsigned int flags;
    float offset[3];
    float orient[3];
    unsigned char unmodeled_24[0xC];
    float fixedScale[3];
    float scale[3];
    unsigned char unmodeled_48[0x4];
    float limit[6];
    unsigned char unmodeled_64[0xC];
} TiHrcSkip;

extern const unsigned char D_0068F3E0[];
extern const unsigned char tiMotionAssertParentScale[];

/* Scale lookup for node `index` in the pMRPack table at +0xD0; asserts "can not found parent scale". */
float *tiMotionFindParentScale(TiMotionPackage *pMRPack, int index)
{
    TiHrcData *hrc;
    TiHrcSkip *skips;
    TiHrcDynamics *dynamics;
    unsigned char *vectors;
    int vectorBytes;
    int dynamicsIndex;
    unsigned int skipEnd;
    int steps;
    unsigned int base;
    unsigned int rangeEnd;

    if (index < 0) {
        return 0;
    }
    if (index < TI_LOCATOR_COUNT) {
        return pMRPack->pHrcData->locators[index].scale;
    }
    hrc = pMRPack->pHrcData;
    if ((unsigned int) index < (unsigned int) hrc->jointCount) {
        return hrc->joints[index - TI_LOCATOR_COUNT].scale;
    }
    base = hrc->jointCount;
    rangeEnd = base + hrc->joint2Count;
    if (index < rangeEnd) {
        TiHrcJoint *joint2 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint2Offset);
        return joint2[index - base].scale;
    }
    base = rangeEnd;
    if (index < base + hrc->joint3Count) {
        TiHrcJoint *joint3 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint3Offset);
        return joint3[index - base].scale;
    }
    base += hrc->joint3Count;
    skipEnd = base + hrc->skipCount;
    if (index < skipEnd) {
        skips = (TiHrcSkip *) ((unsigned char *) hrc + hrc->skipOffset);
        return skips[index - base].scale;
    }
    base = skipEnd;
    if (index < base + hrc->dynamicsCount) {
        skipEnd = base + hrc->dynamicsCount;
        dynamicsIndex = index - base;
        dynamics = (TiHrcDynamics *) ((unsigned char *) hrc + hrc->dynamicsOffset);
        if (dynamicsIndex > 0) {
            steps = dynamicsIndex;
            do {
                vectorBytes = (dynamics->vectorCount[0] + dynamics->vectorCount[1]
                               + dynamics->vectorCount[2]) * 16;
                vectors = (unsigned char *) dynamics->vectors;
                dynamics = (TiHrcDynamics *) (vectors + vectorBytes);
            } while (--steps != 0);
        }
        return dynamics->scale;
    }
    if (D_0068F3E0 != 0) {
        assertFailed(tiMotionAssertParentScale, tiMotionSourceFile, 165);
    }
    return 0;
}

/* Identity, then offset, rotation Z/Y/X and
 * scale, each applied only when it is not zero (or one for scale); stores the result in `matrix`. Inline: the
 * solver below expands it, and the compiler still emits it, at the end of the file. */
inline void tiMotionTransformToMatrix(Matrix4 matrix, TiMotionTransform *transform)
{
    tiVu0LoadFixedMatrix();
    if (transform->offset[0] != 0.0f || transform->offset[1] != 0.0f || transform->offset[2] != 0.0f) {
        xglCurMulTranslate(transform->offset);
    }
    if (transform->rotation[2] != 0.0f) {
        xglCurMulRotateZ(transform->rotation[2]);
    }
    if (transform->rotation[1] != 0.0f) {
        xglCurMulRotateY(transform->rotation[1]);
    }
    if (transform->rotation[0] != 0.0f) {
        xglCurMulRotateX(transform->rotation[0]);
    }
    if (transform->scale[0] != 1.0f || transform->scale[1] != 1.0f || transform->scale[2] != 1.0f) {
        xglCurMulScaleXYZ(transform->scale);
    }
    xglCurMatrixGet(matrix);
}

/* Translation values[0..2], applied when any is not zero. */
#define TI_MUL_TRANSLATE(values)                                                          \
    if ((values)[0] != 0.0f || (values)[1] != 0.0f || (values)[2] != 0.0f) {              \
        xglCurMulTranslate(values);                                                       \
    }
/* Undo the parent's scale when the node asks for it (flag 2). */
#define TI_MUL_PARENT_INV_SCALE(pMRPack, node)                                            \
    if ((node)->flags & TI_NODE_FLAG_INV_PARENT_SCALE) {                                                              \
        float *parentScale = tiMotionFindParentScale(pMRPack, (node)->parent);           \
        if (parentScale != 0                                                              \
            && (parentScale[0] != 1.0f || parentScale[1] != 1.0f || parentScale[2] != 1.0f)) { \
            xglCurMulInvScaleXYZ(parentScale);                                            \
        }                                                                                 \
    }
/* Rotation Z, Y then X, each applied when it is not zero. */
#define TI_MUL_ROTATE(rotation)                                                           \
    if ((rotation)[2] != 0.0f) {                                                          \
        xglCurMulRotateZ((rotation)[2]);                                                  \
    }                                                                                     \
    if ((rotation)[1] != 0.0f) {                                                          \
        xglCurMulRotateY((rotation)[1]);                                                  \
    }                                                                                     \
    if ((rotation)[0] != 0.0f) {                                                          \
        xglCurMulRotateX((rotation)[0]);                                                  \
    }
/* Scale values[6..8]: a non-unit scale marks the package (0x20) and is applied; the node keeps it. */
#define TI_MUL_SCALE(pMRPack, values, node)                                               \
    if ((values)[6] != 1.0f || (values)[7] != 1.0f || (values)[8] != 1.0f) {              \
        tiWordSetBits(&(pMRPack)->flags, TI_PACKAGE_FLAG_SCALED);                                           \
        xglCurMulScaleXYZ(&(values)[6]);                                                  \
    }                                                                                     \
    (node)->scale[0] = (values)[6];                                                       \
    (node)->scale[1] = (values)[7];                                                       \
    (node)->scale[2] = (values)[8];

/* Evaluates one locator into `matrix`. */
static inline void tiMotionSolveLocator(TiMotionPackage *pMRPack, Matrix4 matrix, TiHrcLocator *locator,
                                        unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransLocator(values, locator, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, locator);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, locator);
    xglCurMatrixGet(matrix);
}

/* Evaluates one joint into `matrix`; the joint's rotation scale scales the animated rotation. */
static inline void tiMotionSolveJoint(TiMotionPackage *pMRPack, Matrix4 matrix, TiHrcJoint *joint,
                                      unsigned char **ppCursor)
{
    float values[9];
    unsigned int k;

    xacEvalTransJoint(values, joint, ppCursor, pMRPack->frame);
    if (joint->rotationScale != 1.0f) {
        for (k = 3; k < 6; k++) {
            values[k] *= joint->rotationScale;
        }
    }
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, joint);
    TI_MUL_ROTATE(joint->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, joint);
    xglCurMatrixGet(matrix);
}

/* Evaluates one dynamics node into `matrix`. */
static inline void tiMotionSolveDynamics(TiMotionPackage *pMRPack, Matrix4 matrix, TiHrcDynamics *dynamics,
                                         unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransDynamics(values, dynamics, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, dynamics);
    TI_MUL_ROTATE(dynamics->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, dynamics);
    xglCurMatrixGet(matrix);
}

/* Same as tiMotionSolveDynamics for animation data older than version 1.3. */
static inline void tiMotionSolveDynamicsOld(TiMotionPackage *pMRPack, Matrix4 matrix, TiHrcDynamics *dynamics,
                                            unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransDynamicsNoWeight(values, dynamics, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, dynamics);
    TI_MUL_ROTATE(dynamics->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, dynamics);
    xglCurMatrixGet(matrix);
}

/* For animation data older than version 1.4 (tiMRPackageUpdate picks it). Evaluates the animation of every node of the
 * hierarchy and stores its local matrix in pMRPack->pLocalMat, in node order: the two locators,
 * the joints, two further joint tables, the dynamics nodes and the static nodes. */
void tiMotionSolveLocalMatricesOld(TiMotionPackage *pMRPack)
{
    unsigned char *cursor = 0;
    unsigned char **ppCursor;
    TiHrcData *hrc;
    TiHrcLocator *locators;
    TiHrcJoint *joints;
    TiHrcJoint *joint2;
    TiHrcJoint *joint3;
    TiHrcDynamics *dynamics;
    TiHrcStatic *statics;
    TiXacData *anim;
    unsigned int nodeIndex;
    unsigned int i;
    int vectorBytes;
    unsigned char *vectors; /* original temporary: removing it changes the address add */

    hrc = pMRPack->pHrcData;
    locators = hrc->locators;
    joints = hrc->joints;
    joint2 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint2Offset);
    joint3 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint3Offset);
    dynamics = (TiHrcDynamics *) ((unsigned char *) hrc + hrc->dynamicsOffset);
    statics = (TiHrcStatic *) ((unsigned char *) hrc + hrc->staticOffset);
    anim = pMRPack->pAnimData;
    if (pMRPack->pLocalMat == 0) {
        assertFailed(tiMotionAssertPLocalMat, tiMotionSourceFile, 0x1EE);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(tiMotionAssertPHrcData, tiMotionSourceFile, 0x1EF);
    }

    ppCursor = &cursor;
    nodeIndex = 0;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->jointOffset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < TI_LOCATOR_COUNT; i++, nodeIndex++) {
        tiMotionSolveLocator(pMRPack, pMRPack->pLocalMat[nodeIndex], &locators[i], ppCursor);
    }
    pMRPack->pLocalMat[1][3][0] *= pMRPack->scale[0];
    pMRPack->pLocalMat[1][3][1] *= pMRPack->scale[1];
    pMRPack->pLocalMat[1][3][2] *= pMRPack->scale[2];

    for (i = 0; i < pMRPack->pHrcData->jointCount - TI_LOCATOR_COUNT; i++, nodeIndex++) {
        if (cursor != 0 && i >= anim->jointCount - TI_LOCATOR_COUNT) {
            cursor = 0;
        }
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joints[i], ppCursor);
    }

    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->joint2Offset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < pMRPack->pHrcData->joint2Count; i++, nodeIndex++) {
        if (cursor != 0 && i >= anim->joint2Count) {
            cursor = 0;
        }
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joint2[i], ppCursor);
    }

    *ppCursor = 0;
    for (i = 0; i < pMRPack->pHrcData->joint3Count; i++, nodeIndex++) {
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joint3[i], ppCursor);
    }

    nodeIndex += pMRPack->pHrcData->skipCount;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->dynamicsOffset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < pMRPack->pHrcData->dynamicsCount; i++) {
        vectorBytes = (dynamics->vectorCount[0] + dynamics->vectorCount[1] + dynamics->vectorCount[2]) * 16;
        if (cursor != 0 && i >= anim->dynamicsCount) {
            cursor = 0;
        }
        if (anim != 0) {
            if (anim->version >= 1.3f) {
                tiMotionSolveDynamics(pMRPack, pMRPack->pLocalMat[nodeIndex], dynamics, ppCursor);
            } else {
                dynamics->weight = 0.5f;
                tiMotionSolveDynamicsOld(pMRPack, pMRPack->pLocalMat[nodeIndex], dynamics, ppCursor);
            }
        } else {
            tiMotionSolveDynamics(pMRPack, pMRPack->pLocalMat[nodeIndex], dynamics, ppCursor);
        }
        nodeIndex++;
        vectors = (unsigned char *) dynamics->vectors;
        dynamics = (TiHrcDynamics *) (vectors + vectorBytes);
    }

    for (i = 0; i < pMRPack->pHrcData->staticCount; i++, nodeIndex++) {
        tiMotionTransformToMatrix(pMRPack->pLocalMat[nodeIndex], &statics[i].transform);
    }
}

typedef struct TiXacData2 {
    int unmodeled_00;
    float version;
    unsigned char unmodeled_08[0x8];
    int jointCount;
    int joint2Count;
    int skipCount;
    int dynamicsCount;
    unsigned int visibilityCount;
    int joint3Count;
    int skipOffset;
    int unmodeled_2C;
    int jointOffset;
    int joint2Offset;
    int unmodeled_38;
    int dynamicsOffset;
    int visibilityOffset;
    int joint3Offset;
} TiXacData2;

/* Same walk as tiMotionSolveLocalMatricesOld for animation data of the second format: the third joint table is
 * animated too, every dynamics node uses the current solver, and the static nodes are solved only
 * when the hierarchy has a static table. */
void tiMotionSolveLocalMatrices(TiMotionPackage *pMRPack)
{
    unsigned char *cursor = 0;
    unsigned char **ppCursor;
    TiHrcData *hrc;
    TiHrcLocator *locators;
    TiHrcJoint *joints;
    TiHrcJoint *joint2;
    TiHrcJoint *joint3;
    TiHrcDynamics *dynamics;
    TiHrcStatic *statics;
    TiXacData2 *anim;
    unsigned int nodeIndex;
    unsigned int i;
    int vectorBytes;
    unsigned char *vectors; /* original temporary: removing it changes the address add */

    hrc = pMRPack->pHrcData;
    locators = hrc->locators;
    joints = hrc->joints;
    joint2 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint2Offset);
    joint3 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint3Offset);
    dynamics = (TiHrcDynamics *) ((unsigned char *) hrc + hrc->dynamicsOffset);
    statics = (TiHrcStatic *) ((unsigned char *) hrc + hrc->staticOffset);
    anim = pMRPack->pAnimData;
    if (pMRPack->pLocalMat == 0) {
        assertFailed(tiMotionAssertPLocalMat, tiMotionSourceFile, 0x244);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(tiMotionAssertPHrcData, tiMotionSourceFile, 0x245);
    }

    ppCursor = &cursor;
    nodeIndex = 0;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->jointOffset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < TI_LOCATOR_COUNT; i++, nodeIndex++) {
        tiMotionSolveLocator(pMRPack, pMRPack->pLocalMat[nodeIndex], &locators[i], ppCursor);
    }
    pMRPack->pLocalMat[1][3][0] *= pMRPack->scale[0];
    pMRPack->pLocalMat[1][3][1] *= pMRPack->scale[1];
    pMRPack->pLocalMat[1][3][2] *= pMRPack->scale[2];

    for (i = 0; i < pMRPack->pHrcData->jointCount - TI_LOCATOR_COUNT; i++, nodeIndex++) {
        if (cursor != 0 && i >= anim->jointCount - TI_LOCATOR_COUNT) {
            cursor = 0;
        }
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joints[i], ppCursor);
    }

    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->joint2Offset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < pMRPack->pHrcData->joint2Count; i++, nodeIndex++) {
        if (cursor != 0 && i >= anim->joint2Count) {
            cursor = 0;
        }
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joint2[i], ppCursor);
    }

    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->joint3Offset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < pMRPack->pHrcData->joint3Count; i++, nodeIndex++) {
        if (cursor != 0 && i >= anim->joint3Count) {
            cursor = 0;
        }
        tiMotionSolveJoint(pMRPack, pMRPack->pLocalMat[nodeIndex], &joint3[i], ppCursor);
    }

    nodeIndex += pMRPack->pHrcData->skipCount;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->dynamicsOffset;
    } else {
        cursor = 0;
    }
    for (i = 0; i < pMRPack->pHrcData->dynamicsCount; i++) {
        vectorBytes = (dynamics->vectorCount[0] + dynamics->vectorCount[1] + dynamics->vectorCount[2]) * 16;
        if (cursor != 0 && i >= anim->dynamicsCount) {
            cursor = 0;
        }
        tiMotionSolveDynamics(pMRPack, pMRPack->pLocalMat[nodeIndex], dynamics, ppCursor);
        nodeIndex++;
        vectors = (unsigned char *) dynamics->vectors;
        dynamics = (TiHrcDynamics *) (vectors + vectorBytes);
    }

    if (pMRPack->pHrcData->staticOffset != 0) {
        for (i = 0; i < pMRPack->pHrcData->staticCount; i++, nodeIndex++) {
            tiMotionTransformToMatrix(pMRPack->pLocalMat[nodeIndex], &statics[i].transform);
        }
    }
}

void xacEvalTransFace(float *values, TiHrcSkip *node, unsigned char **cursor, float frame);

/* Solves the skip nodes of the hierarchy: each node of the table at hrc->skipOffset gets the local matrix
 * after the joint tables, with its parent scale, orientation, angle limits and scale. */
void tiMotionSolveSkipNodesOld(TiMotionPackage *pMRPack)
{
    TiHrcData *hrc;
    TiXacData2 *anim;
    TiHrcSkip *skips;
    TiHrcSkip *node;
    unsigned char *cursor = 0;
    unsigned char **ppCursor;
    unsigned int nodeIndex;
    unsigned int i;
    float values[9];
    Matrix4 *matrix;
    unsigned int matrixOffset;

    hrc = pMRPack->pHrcData;
    anim = pMRPack->pSkipAnimData;
    skips = (TiHrcSkip *) ((unsigned char *) hrc + hrc->skipOffset);
    if (pMRPack->pLocalMat == 0) {
        assertFailed(tiMotionAssertPLocalMat, tiMotionSourceFile, 657);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(tiMotionAssertPHrcData, tiMotionSourceFile, 658);
    }

    nodeIndex = pMRPack->pHrcData->jointCount + pMRPack->pHrcData->joint2Count + pMRPack->pHrcData->joint3Count;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->skipOffset;
    } else {
        cursor = 0;
    }
    if (pMRPack->pHrcData->skipOffset != 0) {
        {
        for (i = 0; i < pMRPack->pHrcData->skipCount; i++, nodeIndex++) {
            ppCursor = &cursor;
            if (cursor != 0 && i >= anim->skipCount) {
                cursor = 0;
            }
            /* Address by byte offset (nodeIndex << 6): the indexed form changes the code. */
            matrixOffset = nodeIndex << 6;
            matrix = (Matrix4 *) (matrixOffset + (unsigned int) pMRPack->pLocalMat);
            xacEvalTransFace(values, &skips[i], ppCursor, pMRPack->skipFrame);
            tiVu0LoadFixedMatrix();
            node = &skips[i];
            if (node->offset[0] != 0.0f || node->offset[1] != 0.0f || node->offset[2] != 0.0f) {
                xglCurMulTranslate(values);
            }
            TI_MUL_PARENT_INV_SCALE(pMRPack, node);
            TI_MUL_ROTATE(node->orient);
            TI_MUL_ROTATE(&values[3]);
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_X_MIN) {
                if (values[3] < node->limit[0]) {
                    values[3] = node->limit[0];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_X_MAX) {
                if (node->limit[1] < values[3]) {
                    values[3] = node->limit[1];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Y_MIN) {
                if (values[4] < node->limit[2]) {
                    values[4] = node->limit[2];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Y_MAX) {
                if (node->limit[3] < values[4]) {
                    values[4] = node->limit[3];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Z_MIN) {
                if (values[5] < node->limit[4]) {
                    values[5] = node->limit[4];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Z_MAX) {
                if (node->limit[5] < values[5]) {
                    values[5] = node->limit[5];
                }
            }
            if (node->fixedScale[0] != 1.0f || node->fixedScale[1] != 1.0f
                || node->fixedScale[2] != 1.0f) {
                tiWordSetBits(&pMRPack->skipFlags, TI_PACKAGE_FLAG_SCALED);
                xglCurMulScaleXYZ(&values[6]);
            }
            node->scale[0] = values[6];
            node->scale[1] = values[7];
            node->scale[2] = values[8];
            xglCurMatrixGet(*matrix);
        }
    }
    }
}

void tiMotionSolveSkipNodes(TiMotionPackage *pMRPack)
{
    TiHrcData *hrc;
    TiXacData2 *anim;
    TiHrcSkip *skips;
    TiHrcSkip *node;
    unsigned char *cursor = 0;
    unsigned char **ppCursor;
    unsigned int nodeIndex;
    unsigned int i;
    float values[9];
    Matrix4 *matrix;
    unsigned int matrixOffset;

    hrc = pMRPack->pHrcData;
    anim = pMRPack->pSkipAnimData;
    skips = (TiHrcSkip *) ((unsigned char *) hrc + hrc->skipOffset);
    if (pMRPack->pLocalMat == 0) {
        assertFailed(tiMotionAssertPLocalMat, tiMotionSourceFile, 686);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(tiMotionAssertPHrcData, tiMotionSourceFile, 687);
    }

    nodeIndex = pMRPack->pHrcData->jointCount + pMRPack->pHrcData->joint2Count + pMRPack->pHrcData->joint3Count;
    if (anim != 0) {
        cursor = (unsigned char *) anim + anim->unmodeled_38;
    } else {
        cursor = 0;
    }
    if (pMRPack->pHrcData->skipOffset != 0) {
        {
        for (i = 0; i < pMRPack->pHrcData->skipCount; i++, nodeIndex++) {
            ppCursor = &cursor;
            if (cursor != 0 && i >= anim->skipCount) {
                cursor = 0;
            }
            /* Address by byte offset (nodeIndex << 6): the indexed form changes the code. */
            matrixOffset = nodeIndex << 6;
            matrix = (Matrix4 *) (matrixOffset + (unsigned int) pMRPack->pLocalMat);
            xacEvalTransFace(values, &skips[i], ppCursor, pMRPack->skipFrame);
            tiVu0LoadFixedMatrix();
            node = &skips[i];
            if (node->offset[0] != 0.0f || node->offset[1] != 0.0f || node->offset[2] != 0.0f) {
                xglCurMulTranslate(values);
            }
            TI_MUL_PARENT_INV_SCALE(pMRPack, node);
            TI_MUL_ROTATE(node->orient);
            TI_MUL_ROTATE(&values[3]);
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_X_MIN) {
                if (values[3] < node->limit[0]) {
                    values[3] = node->limit[0];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_X_MAX) {
                if (node->limit[1] < values[3]) {
                    values[3] = node->limit[1];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Y_MIN) {
                if (values[4] < node->limit[2]) {
                    values[4] = node->limit[2];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Y_MAX) {
                if (node->limit[3] < values[4]) {
                    values[4] = node->limit[3];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Z_MIN) {
                if (values[5] < node->limit[4]) {
                    values[5] = node->limit[4];
                }
            }
            if (node->limitFlags & TI_SKIP_LIMIT_ROT_Z_MAX) {
                if (node->limit[5] < values[5]) {
                    values[5] = node->limit[5];
                }
            }
            if (node->fixedScale[0] != 1.0f || node->fixedScale[1] != 1.0f
                || node->fixedScale[2] != 1.0f) {
                tiWordSetBits(&pMRPack->skipFlags, TI_PACKAGE_FLAG_SCALED);
                xglCurMulScaleXYZ(&values[6]);
            }
            node->scale[0] = values[6];
            node->scale[1] = values[7];
            node->scale[2] = values[8];
            xglCurMatrixGet(*matrix);
        }
    }
    }
}

void xglCurMatrixSet(Matrix4 matrix);
TiHrcDynamics *tiDynamicsNodeSolveNext(Matrix4 localMat, TiMotionPackage *pMRPack, TiHrcDynamics *dynamics, Matrix4 dstMat);
void tiWindSolveCellPairs(TiMotionPackage *pMRPack);

/* Chains the dynamics records of the hierarchy: each record takes the world matrix of its parent (or the fixed matrix when the parent is negative), multiplies in its local matrix and stores the result in pWorldMat. A nonzero weight copies the translation of that world matrix into pDynMat. */
void tiMotionChainDynamics(TiMotionPackage *pMRPack, TiHrcDynamics *dynamics)
{
    TiHrcData *hrc;
    TiHrcDynamics *record;
    unsigned int nodeIndex;
    unsigned int dynamicsIndex;
    int parent;
    int leading;

    hrc = pMRPack->pHrcData;
    leading = hrc->jointCount + hrc->skipCount;
    nodeIndex = leading + (hrc->joint3Count + hrc->joint2Count);
    dynamicsIndex = 0;
    if (hrc->dynamicsCount != 0) {
        do {
            record = dynamics;
            parent = record->parent;
            dynamics = tiDynamicsNodeSolveNext(pMRPack->pLocalMat[nodeIndex], pMRPack, record, pMRPack->pDynMat[dynamicsIndex].matrix);
            if (parent >= 0) {
                xglCurMatrixSet(pMRPack->pWorldMat[parent]);
            } else {
                tiVu0LoadFixedMatrix();
            }
            xglCurMatrixMul(pMRPack->pLocalMat[nodeIndex]);
            xglCurMatrixGet(pMRPack->pWorldMat[nodeIndex]);
            if (record->weight != 0.0f) {
                pMRPack->pDynMat[dynamicsIndex].matrix[1][0] = pMRPack->pWorldMat[nodeIndex][3][0];
                pMRPack->pDynMat[dynamicsIndex].matrix[1][1] = pMRPack->pWorldMat[nodeIndex][3][1];
                pMRPack->pDynMat[dynamicsIndex].matrix[1][2] = pMRPack->pWorldMat[nodeIndex][3][2];
            }
            nodeIndex++;
            dynamicsIndex++;
        } while (dynamicsIndex < pMRPack->pHrcData->dynamicsCount);
    }
    tiWindSolveCellPairs(pMRPack);
}

extern const unsigned char tiMotionAssertPGlobalMat[];
void tiMtnBlendUpdateGlobal(TiMotionPackage *pMRPack);
void tiDynamicsCollTransformVectors(TiMotionPackage *pMRPack);
void tiDynamicsCollBuildMatrices(TiMotionPackage *pMRPack);
void tiDynamicsCollBuildWideMatrices(TiMotionPackage *pMRPack);

/* Chains one node: its world matrix is its parent's world matrix (or the fixed matrix for a root) times its local matrix. */
static inline void tiMotionChainNode(TiMotionPackage *pMRPack, int parent, unsigned int nodeIndex)
{
    if (parent >= 0) {
        xglCurMatrixSet(pMRPack->pWorldMat[parent]);
    } else {
        tiVu0LoadFixedMatrix();
    }
    xglCurMatrixMul(pMRPack->pLocalMat[nodeIndex]);
    xglCurMatrixGet(pMRPack->pWorldMat[nodeIndex]);
}

/* Chains the world matrices of every node group, then the dynamics, and finishes the collision and blend passes. */
void tiMotionUpdateGlobal(TiMotionPackage *pMRPack)
{
    TiHrcData *hrc;
    TiHrcLocator *locators;
    TiHrcJoint *joints;
    TiHrcJoint *joint2;
    TiHrcJoint *joint3;
    TiHrcSkip *skips;
    TiHrcDynamics *dynamics;
    TiHrcStatic *statics;
    unsigned int nodeIndex;
    unsigned int i;

    hrc = pMRPack->pHrcData;
    locators = hrc->locators;
    joints = hrc->joints;
    joint2 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint2Offset);
    joint3 = (TiHrcJoint *) ((unsigned char *) hrc + hrc->joint3Offset);
    skips = (TiHrcSkip *) ((unsigned char *) hrc + hrc->skipOffset);
    dynamics = (TiHrcDynamics *) ((unsigned char *) hrc + hrc->dynamicsOffset);
    statics = (TiHrcStatic *) ((unsigned char *) hrc + hrc->staticOffset);
    if (pMRPack->pWorldMat == 0) {
        assertFailed(tiMotionAssertPGlobalMat, tiMotionSourceFile, 0x2F1);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(tiMotionAssertPHrcData, tiMotionSourceFile, 0x2F2);
    }

    for (i = 0, nodeIndex = 0; i < TI_LOCATOR_COUNT; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, locators[i].parent, nodeIndex);
    }
    for (i = 0; i < pMRPack->pHrcData->jointCount - TI_LOCATOR_COUNT; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, joints[i].parent, nodeIndex);
        if (nodeIndex == pMRPack->targetNode) {
            /* The target is the package's flags word (+0x10 overlays TiMotionTarget): a cast, no code. */
            tiMotionAimTarget(pMRPack->pWorldMat[nodeIndex], pMRPack->pPlaceMat, (TiMotionTarget *) &pMRPack->flags, joints[i].range);
        }
    }
    for (i = 0; i < pMRPack->pHrcData->joint2Count; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, joint2[i].parent, nodeIndex);
    }
    for (i = 0; i < pMRPack->pHrcData->joint3Count; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, joint3[i].parent, nodeIndex);
    }
    for (i = 0; i < pMRPack->pHrcData->skipCount; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, skips[i].parent, nodeIndex);
    }
    nodeIndex += pMRPack->pHrcData->dynamicsCount;
    for (i = 0; i < pMRPack->pHrcData->staticCount; i++, nodeIndex++) {
        tiMotionChainNode(pMRPack, statics[i].transform.parent, nodeIndex);
    }
    tiMtnBlendUpdateGlobal(pMRPack);
    tiDynamicsCollTransformVectors(pMRPack);
    tiDynamicsCollBuildMatrices(pMRPack);
    tiDynamicsCollBuildWideMatrices(pMRPack);
    tiMotionChainDynamics(pMRPack, dynamics);
}

extern const unsigned char tiMotionAssertPMRPack[];
extern const unsigned char tiMotionAssertPVisible[];
unsigned char *xacFindCurveKey(int *flag, unsigned char *keyCursor, float frame);

/* Sets or clears one visibility bit per curve of the animation's visibility table (+0x40, count +0x20): a set flag from
 * xacFindCurveKey clears the bit. pVisible is the actor's byte array of 32-bit visibility words. Animation data of the second format; version 1.4 or later only. */
void tiMotionUpdateVisibility(TiMotionPackage *pMRPack, unsigned char *pVisible)
{
    TiXacData2 *anim;
    unsigned char *cursor;
    int hidden;
    int curveIndex;

    if (pMRPack == 0) {
        assertFailed(tiMotionAssertPMRPack, tiMotionSourceFile, 852);
    }
    if (pVisible == 0) {
        assertFailed(tiMotionAssertPVisible, tiMotionSourceFile, 853);
    }
    anim = pMRPack->pAnimData;
    if (anim == 0 || anim->version < 1.4f) {
        return;
    }
    curveIndex = 0;
    cursor = (unsigned char *) anim + anim->visibilityOffset;
    if (anim->visibilityCount != 0) {
        do {
            cursor = xacFindCurveKey(&hidden, cursor, pMRPack->frame);
            if (hidden != 0) {
                /* Word access through the byte pointer: an int array index changes the code. */
                *(int *) (pVisible + ((curveIndex >> 5) << 2)) &= ~(1 << (curveIndex & 0x1F));
            } else {
                *(int *) (pVisible + ((curveIndex >> 5) << 2)) |= 1 << (curveIndex & 0x1F);
            }
            curveIndex++;
        /* Reloads the count through pMRPack (anim->visibilityCount changes the code). */
        } while (curveIndex < ((TiXacData2 *) pMRPack->pAnimData)->visibilityCount);
    }
}

typedef struct TiMotionOffsetBlock {
    unsigned char unmodeled_00[0x30];
    int joint_header_offset;
} TiMotionOffsetBlock;

void xacEvalJointCurves9(int res, void *joint_header, float value);

/* Thin wrapper: the joint header sits at block + block->joint_header_offset. */
void tiMotionSampleJoint(int res, TiMotionOffsetBlock *block, float value)
{
    xacEvalJointCurves9(res, (unsigned char *) block + block->joint_header_offset, value);
}

/* Out-of-line copy of tiMotionSolveLocator: evaluates one locator into `matrix`. */
void tiMotionSolveLocatorOutOfLine(Matrix4 matrix, TiMotionPackage *pMRPack, TiHrcLocator *locator, unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransLocator(values, locator, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, locator);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, locator);
    xglCurMatrixGet(matrix);
}

/* Out-of-line copy of tiMotionSolveJoint: evaluates one joint into `matrix`. */
void tiMotionSolveJointOutOfLine(Matrix4 matrix, TiMotionPackage *pMRPack, TiHrcJoint *joint, unsigned char **ppCursor)
{
    float values[9];
    unsigned int k;

    xacEvalTransJoint(values, joint, ppCursor, pMRPack->frame);
    if (joint->rotationScale != 1.0f) {
        for (k = 3; k < 6; k++) {
            values[k] *= joint->rotationScale;
        }
    }
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, joint);
    TI_MUL_ROTATE(joint->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, joint);
    xglCurMatrixGet(matrix);
}

/* Evaluates one skip (face) node into `matrix`: the per-node body of the skip loops above, with the face limits. */
void tiMotionSolveFace(Matrix4 matrix, TiMotionPackage *pMRPack, TiHrcSkip *face, unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransFace(values, face, ppCursor, pMRPack->skipFrame);
    tiVu0LoadFixedMatrix();
    if (face->offset[0] != 0.0f || face->offset[1] != 0.0f || face->offset[2] != 0.0f) {
        xglCurMulTranslate(values);
    }
    TI_MUL_PARENT_INV_SCALE(pMRPack, face);
    TI_MUL_ROTATE(face->orient);
    TI_MUL_ROTATE(&values[3]);
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_X_MIN) != 0) {
        if (values[3] < face->limit[0]) {
            values[3] = face->limit[0];
        }
    }
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_X_MAX) != 0) {
        if (face->limit[1] < values[3]) {
            values[3] = face->limit[1];
        }
    }
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_Y_MIN) != 0) {
        if (values[4] < face->limit[2]) {
            values[4] = face->limit[2];
        }
    }
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_Y_MAX) != 0) {
        if (face->limit[3] < values[4]) {
            values[4] = face->limit[3];
        }
    }
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_Z_MIN) != 0) {
        if (values[5] < face->limit[4]) {
            values[5] = face->limit[4];
        }
    }
    if ((face->limitFlags & TI_SKIP_LIMIT_ROT_Z_MAX) != 0) {
        if (face->limit[5] < values[5]) {
            values[5] = face->limit[5];
        }
    }
    if (face->fixedScale[0] != 1.0f || face->fixedScale[1] != 1.0f || face->fixedScale[2] != 1.0f) {
        tiWordSetBits(&pMRPack->skipFlags, TI_PACKAGE_FLAG_SCALED);
        xglCurMulScaleXYZ(&values[6]);
    }
    face->scale[0] = values[6];
    face->scale[1] = values[7];
    face->scale[2] = values[8];
    xglCurMatrixGet(matrix);
}

void tiMotionSolveDynamicsOutOfLine(Matrix4 matrix, TiMotionPackage *pMRPack, TiHrcDynamics *dynamics, unsigned char **ppCursor)
{
    float values[9];

    xacEvalTransDynamics(values, dynamics, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, dynamics);
    TI_MUL_ROTATE(dynamics->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, dynamics);
    xglCurMatrixGet(matrix);
}

void tiMotionSolveDynamicsOldOutOfLine(Matrix4 matrix, TiMotionPackage *pMRPack, TiHrcDynamics *dynamics, unsigned char **ppCursor)
{
    float values[9];

    dynamics->weight = 0.5f;
    xacEvalTransDynamicsNoWeight(values, dynamics, ppCursor, pMRPack->frame);
    tiVu0LoadFixedMatrix();
    TI_MUL_TRANSLATE(values);
    TI_MUL_PARENT_INV_SCALE(pMRPack, dynamics);
    TI_MUL_ROTATE(dynamics->orient);
    TI_MUL_ROTATE(&values[3]);
    TI_MUL_SCALE(pMRPack, values, dynamics);
    xglCurMatrixGet(matrix);
}

/* Assert texts of the solver (the original .rodata strings, in address order). */
const unsigned char tiMotionAssertPPlaceMat[16] = "pPlaceMat";
const unsigned char tiMotionSourceFile[24] = "../TiMotionSolver.euc.c";
const unsigned char D_0068F3E0[32] = "can not found parent scale";
const unsigned char tiMotionAssertParentScale[32] = "!\"can not found parent scale\"";
const unsigned char tiMotionAssertPLocalMat[24] = "pMRPack->pLocalMat";
const unsigned char tiMotionAssertPHrcData[24] = "pMRPack->pHrcData";
const unsigned char tiMotionAssertPGlobalMat[24] = "pMRPack->pGlobalMat";
const unsigned char tiMotionAssertPMRPack[8] = "pMRPack";
const unsigned char tiMotionAssertPVisible[16] = "pVisible";
