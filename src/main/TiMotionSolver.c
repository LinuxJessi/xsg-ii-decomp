#include "common.h"
#include "shared.h"
#include "main/tu026.h"
#include "main/tu254.h"
#include "main/tu256.h"
#include "main/TiMRPackage.h"


/* TiMotionTarget.flags: the target applies its angle. */
#define TI_TARGET_FLAG_ANGLE 0x10
/* TiHrc* node flags: undo the parent's scale. */
#define TI_NODE_FLAG_INV_PARENT_SCALE 2
/* TiMRPackage.flags: some node has a non-unit scale. */
#define TI_PACKAGE_FLAG_SCALED 0x20
/* Every hierarchy starts with two locators, then the joints. */
#define TI_LOCATOR_COUNT 2

void xglCurMatrixGet(Matrix4 matrix);
void xglCurSetRotateX(float value);
void xglMatrixMul(void *destination, void *left, void *right);
extern unsigned char D_0068F3B8[];
extern unsigned char D_0068F3C8[];

typedef struct TiMotionTarget {
    int flags;
    unsigned char unmodeled_04[0x40];
    float angle;
} TiMotionTarget;

/* Named in the proposals: tiMotionApplyTargetAngle. Multiplies `destination` by a rotation of
 * -angle / 2 when the target's angle flag is set and the range is collapsed (range[1] <= range[0]). */
void tiMotionApplyTargetAngle(Matrix4 destination, void *pPlaceMat, TiMotionTarget *target, const float *range)
{
    Matrix4 delta;

    if (pPlaceMat == 0) {
        assertFailed(D_0068F3B8, D_0068F3C8, 14);
    }
    if ((target->flags & TI_TARGET_FLAG_ANGLE) == 0 || range[1] > range[0]) {
        return;
    }
    xglCurSetRotateX(-target->angle * 0.5f);
    xglCurMatrixGet(delta);
    xglMatrixMul(destination, destination, delta);
}

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_0028D890);

/* Scale lookup for node `index` in the pMRPack table at +0xD0; asserts "can not found parent scale". */
INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", tiMotionFindParentScale);

typedef struct TiMotionTransform {
    unsigned char unmodeled_00[4];
    float offset[3];
    float rotation[3];
    float scale[3];
} TiMotionTransform;

void xglCurMulTranslate(void *vector);
void xglCurMulScaleXYZ(void *vector);

/* Named in the proposals: tiMotionTransformToMatrix. Identity, then offset, rotation Z/Y/X and
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
    unsigned char unmodeled_04[0xC];
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
    int unmodeled_30;
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

typedef struct TiMRPackage {
    unsigned char unmodeled_00[0x10];
    int flags;
    int unmodeled_14;
    float frame;
    unsigned char unmodeled_1C[0x94];
    float scale[3];
    unsigned char unmodeled_BC[0x4];
    TiXacData *pAnimData;
    unsigned char unmodeled_C4[0xC];
    TiHrcData *pHrcData;
    unsigned char unmodeled_D4[0x8];
    Matrix4 *pLocalMat;
} TiMRPackage;

extern unsigned char D_0068F420[];
extern unsigned char D_0068F438[];

void xacEvalTransLocator(float *values, TiHrcLocator *locator, unsigned char **cursor, float frame);
void xacEvalTransJoint(float *values, TiHrcJoint *joint, unsigned char **cursor, float frame);
void xacEvalTransDynamics(float *values, TiHrcDynamics *dynamics, unsigned char **cursor, float frame);
void func_002945C8(float *values, TiHrcDynamics *dynamics, unsigned char **cursor, float frame);
float *tiMotionFindParentScale(TiMRPackage *pMRPack, int index);
void xglCurMulInvScaleXYZ(float *scale);

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
static inline void tiMotionSolveLocator(TiMRPackage *pMRPack, Matrix4 matrix, TiHrcLocator *locator,
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
static inline void tiMotionSolveJoint(TiMRPackage *pMRPack, Matrix4 matrix, TiHrcJoint *joint,
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
static inline void tiMotionSolveDynamics(TiMRPackage *pMRPack, Matrix4 matrix, TiHrcDynamics *dynamics,
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
static inline void tiMotionSolveDynamicsOld(TiMRPackage *pMRPack, Matrix4 matrix, TiHrcDynamics *dynamics,
                                            unsigned char **ppCursor)
{
    float values[9];

    func_002945C8(values, dynamics, ppCursor, pMRPack->frame);
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
void tiMotionSolveLocalMatricesOld(TiMRPackage *pMRPack)
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
        assertFailed(D_0068F420, D_0068F3C8, 0x1EE);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(D_0068F438, D_0068F3C8, 0x1EF);
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


/* Animation data of the second format: per-table counts at +0x10 and cursor offsets at +0x30. */
typedef struct TiXacData2 {
    int unmodeled_00;
    float version;
    unsigned char unmodeled_08[0x8];
    int jointCount;
    int joint2Count;
    int unmodeled_18;
    int dynamicsCount;
    int unmodeled_20;
    int joint3Count;
    unsigned char unmodeled_28[0x8];
    int jointOffset;
    int joint2Offset;
    int unmodeled_38;
    int dynamicsOffset;
    int unmodeled_40;
    int joint3Offset;
} TiXacData2;

/* Same walk as tiMotionSolveLocalMatricesOld for animation data of the second format: the third joint table is
 * animated too, every dynamics node uses the current solver, and the static nodes are solved only
 * when the hierarchy has a static table. */
void tiMotionSolveLocalMatrices(TiMRPackage *pMRPack)
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
    anim = (TiXacData2 *) pMRPack->pAnimData;
    if (pMRPack->pLocalMat == 0) {
        assertFailed(D_0068F420, D_0068F3C8, 0x244);
    }
    if (pMRPack->pHrcData == 0) {
        assertFailed(D_0068F438, D_0068F3C8, 0x245);
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

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_002900F8);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_00290518);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_00290938);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", tiMotionUpdateGlobal);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", tiMotionUpdateVisibility);

typedef struct TiMotionOffsetBlock {
    unsigned char unmodeled_00[0x30];
    int joint_header_offset;
} TiMotionOffsetBlock;

void func_00294AE0(int res, void *joint_header, float value);

/* Thin wrapper: the joint header sits at block + block->joint_header_offset. */
void tiMotionSampleJoint(int res, TiMotionOffsetBlock *block, float value)
{
    func_00294AE0(res, (unsigned char *) block + block->joint_header_offset, value);
}

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_00291010);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_002911F8);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_00291478);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_00291780);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_002919C0);
