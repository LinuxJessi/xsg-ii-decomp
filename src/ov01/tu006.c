#include "common.h"
#include "shared.h"

typedef struct StudioCamera {
    u32 active;
    u32 state;
    u8 unmodeled_08[0x18];
    float nearClip;
    float farClip;
    u8 unmodeled_28[0x48];
    Vector4 screenOffset;
    Vector4 screenScale;
    u8 unmodeled_90[4];
    float fov;
    u8 unmodeled_98[8];
    DoublewordVector4 rotation;
    Vector4 angleVelocity;
    u8 unmodeled_c0[0x10];
    DoublewordVector4 position;
    u8 unmodeled_e0[0x40];
    DoublewordVector4 chasePosition;
    u8 unmodeled_130[0x40];
    Matrix4 cameraToWorldMatrix;
    Matrix4 occlusionTransform; /* 0x1B0: read by face_point.c's setup_occlusion/check_occlusion */
    u8 unmodeled_1f0[0x280];
    Matrix4 viewMatrix;
    u8 unmodeled_4b0[0x40];
    Matrix4 clipMatrices[2]; /* 0x4F0: the pair _ModelCalcClipInit loads into vf12..vf19 */
    u8 unmodeled_570[0x80];
} StudioCamera;

extern void xglMatrixStackRotX(float angle);
extern void xglMatrixStackRotY(float angle);
extern void xglMatrixStackRotZ(float angle);
extern void xglMatrixStackSave(float matrix[4][4]);

typedef struct Matrix {
    float elements[16];
} Matrix;

typedef struct FloatVector3 {
    float x;
    float y;
    float z;
} FloatVector3;

extern void xglStudioGetCamera(void *camera_out, int camera_index);
extern void xglMatrixStackLoad(Matrix *source);
extern void grRotMatrix(Matrix *destination, Matrix *source,
                        FloatVector3 *angles);

Matrix *grWorldMatGet(void)
{
    StudioCamera *camera;

    /* Camera zero's view matrix (+0x470) is the current world matrix. */
    xglStudioGetCamera(&camera, 0);
    return (Matrix *)camera->viewMatrix;
}

/* One object's transform record: the translation is position + offset, the angles go to grRotMatrix,
 * and scale is the vector applied last. The last field was named postRotation in the published
 * file; X1 names the same slot scale (xglMatrixScale), and func_001A9F88 sits in that position here. */
typedef struct TransformState {
    FloatVector3 position;
    u8 unmodeled_0c[4];
    FloatVector3 offset;
    u8 unmodeled_1c[4];
    FloatVector3 angles;
    u8 unmodeled_2c[4];
    FloatVector3 scale;
} TransformState;

void xglMatrixUnit(Matrix *matrix);
void func_001AA0CC(Matrix *destination, Matrix *source, const Vector4 *translation);
void func_001A9F88(Matrix *destination, Matrix *source, const FloatVector3 *scale);
extern void xglMatrixMul(Matrix *destination, Matrix *left, Matrix *right);

/* Proposed grCalcMatrix: destination = world * (translate(position + offset) * rotate(angles) * scale).
 * The X1 twin (grCalcMatrix in ov01 gr_gp_init.c) runs the same steps with the same world matrix. */
void grCalcMatrix(TransformState *state, Matrix *destination)
{
    Matrix localMatrix;
    Vector4 translation;
    Matrix *world;

    world = grWorldMatGet();
    xglMatrixUnit(&localMatrix);
    translation.x = state->position.x + state->offset.x;
    translation.y = state->position.y + state->offset.y;
    translation.z = state->position.z + state->offset.z;
    translation.w = 1.0f;
    func_001AA0CC(&localMatrix, &localMatrix, &translation);
    grRotMatrix(&localMatrix, &localMatrix, &state->angles);
    func_001A9F88(&localMatrix, &localMatrix, &state->scale);
    xglMatrixMul(destination, world, &localMatrix);
}

void grRotMatrix(Matrix *destination, Matrix *source, FloatVector3 *angles)
{
    /* The resident matrix stack applies Y, then X, then Z in this wrapper. */
    xglMatrixStackLoad(source);
    xglMatrixStackRotY(angles->y);
    xglMatrixStackRotX(angles->x);
    xglMatrixStackRotZ(angles->z);
    xglMatrixStackSave((float (*)[4])destination);
}

/* Proposed grRotTransPers: transforms the point by the matrix, divides by w, clips to [-1, 1] (flag 0x8000)
 * and maps an in-bounds point to screen X/Y/Z by the camera's screenScale and screenOffset. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu006", grRotTransPers);

/* Proposed grBBCalc: projects the object origin (0, 0, 0, 1) through grCalcMatrix's matrix and returns 0
 * when clipped, else the screen X/Y with the GS offsets 0x7000 and 0x7200 removed and divided by 16. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu006", grBBCalc);

/* Proposed objStdMove: advances the three axes by velocity and acceleration toward the target, snaps an
 * axis on arrival or overshoot (tolerance 1e-6), and returns 1 when all three have arrived. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu006", objStdMove);
