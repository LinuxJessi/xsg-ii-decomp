#include "common.h"
#include "main/tu026.h"
#include "main/tu254.h"
#include "main/tu256.h"

typedef float Matrix4[4][4];

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
 * -angle / 2 when the target's 0x10 flag is set and the range is collapsed (range[1] <= range[0]). */
void tiMotionApplyTargetAngle(Matrix4 destination, void *pPlaceMat, TiMotionTarget *target, const float *range)
{
    Matrix4 delta;

    if (pPlaceMat == 0) {
        assertFailed(D_0068F3B8, D_0068F3C8, 14);
    }
    if ((target->flags & 0x10) == 0 || range[1] > range[0]) {
        return;
    }
    xglCurSetRotateX(-target->angle * 0.5f);
    xglCurMatrixGet(delta);
    xglMatrixMul(destination, destination, delta);
}

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_0028D890);

/* Scale lookup for node `index` in the pMRPack table at +0xD0; asserts "can not found parent scale". */
INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", tiMotionFindParentScale);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_0028DEC0);

INCLUDE_ASM("asm/main/nonmatchings/TiMotionSolver", func_0028F1E0);

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

typedef struct TiMotionTransform {
    unsigned char unmodeled_00[4];
    float offset[3];
    float rotation[3];
    float scale[3];
} TiMotionTransform;

void xglCurMulTranslate(void *vector);
void xglCurMulScaleXYZ(void *vector);

/* Named in the proposals: tiMotionTransformToMatrix. Identity, then offset, rotation Z/Y/X and
 * scale, each applied only when it is not zero (or one for scale); stores the result in `matrix`. */
void tiMotionTransformToMatrix(Matrix4 matrix, TiMotionTransform *transform)
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
