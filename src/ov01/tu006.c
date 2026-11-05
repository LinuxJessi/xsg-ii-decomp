#include "common.h"
#include "shared.h"

typedef float Matrix4[4][4];

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

INCLUDE_ASM("asm/nonmatchings/ov01/tu006", func_00A80E68);

void grRotMatrix(Matrix *destination, Matrix *source, FloatVector3 *angles)
{
    /* The resident matrix stack applies Y, then X, then Z in this wrapper. */
    xglMatrixStackLoad(source);
    xglMatrixStackRotY(angles->y);
    xglMatrixStackRotX(angles->x);
    xglMatrixStackRotZ(angles->z);
    xglMatrixStackSave((float (*)[4])destination);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu006", func_00A80F80);

INCLUDE_ASM("asm/nonmatchings/ov01/tu006", func_00A81128);

INCLUDE_ASM("asm/nonmatchings/ov01/tu006", func_00A811F8);
