#include "common.h"
#include "shared.h"

#define GR_GS_WINDOW_OFFSET_X 0x7000    /* GS X offset of the projected point, 1/16 pixel (X1 gr_gp_init.c) */
#define GR_GS_WINDOW_OFFSET_Y 0x7200    /* GS Y offset of the projected point, 1/16 pixel (X1 gr_gp_init.c) */
#define GR_VERTEX_OUT_OF_FRUSTUM 0x8000 /* clip flag grRotTransPers sets for a point outside [-1, 1] (X1 gr_gp_init.c) */
#define STD_MOVE_ARRIVE_EPSILON 1e-6f   /* objStdMove treats a remaining distance this small as arrived */

/* A 4x4 float matrix, the layout the xgl matrix calls take. */
typedef struct Matrix {
    float elements[4][4];
} Matrix;

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
    Matrix viewMatrix;
    u8 unmodeled_4b0[0x40];
    Matrix4 clipMatrices[2]; /* 0x4F0: the pair _ModelCalcClipInit loads into vf12..vf19 */
    u8 unmodeled_570[0x80];
} StudioCamera;

extern void xglMatrixStackRotX(float angle);
extern void xglMatrixStackRotY(float angle);
extern void xglMatrixStackRotZ(float angle);
extern void xglMatrixStackSave(Matrix *matrix);

typedef struct FloatVector3 {
    float x;
    float y;
    float z;
} FloatVector3;

extern void xglStudioGetCamera(StudioCamera **camera_out, int camera_index);
extern void xglMatrixStackLoad(Matrix *source);
extern void grRotMatrix(Matrix *destination, Matrix *source,
                        FloatVector3 *angles);

Matrix *grWorldMatGet(void)
{
    StudioCamera *camera;

    /* Camera zero's view matrix (+0x470) is the current world matrix. */
    xglStudioGetCamera(&camera, 0);
    return &camera->viewMatrix;
}

/* One object's transform record: the translation is position + offset, the angles go to grRotMatrix,
 * and scale is the vector applied last (xglMatrixScale, as in X1). */
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
void xglMatrixTrans(Matrix *destination, Matrix *source, const Vector4 *translation);
void xglMatrixScale(Matrix *destination, Matrix *source, const FloatVector3 *scale);
extern void xglMatrixMul(Matrix *destination, Matrix *left, Matrix *right);

/* grCalcMatrix: destination = world * (translate(position + offset) * rotate(angles) * scale).
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
    xglMatrixTrans(&localMatrix, &localMatrix, &translation);
    grRotMatrix(&localMatrix, &localMatrix, &state->angles);
    xglMatrixScale(&localMatrix, &localMatrix, &state->scale);
    xglMatrixMul(destination, world, &localMatrix);
}

void grRotMatrix(Matrix *destination, Matrix *source, FloatVector3 *angles)
{
    /* The resident matrix stack applies Y, then X, then Z in this wrapper. */
    xglMatrixStackLoad(source);
    xglMatrixStackRotY(angles->y);
    xglMatrixStackRotX(angles->x);
    xglMatrixStackRotZ(angles->z);
    xglMatrixStackSave(destination);
}

/* Screen-space point written by grRotTransPers: x, y, z and the clip flag word (0x8000 when outside). */
typedef union ScreenPoint {
    struct {
        int x;
        int y;
        int z;
        int flags;
    } xyz;
    unsigned long long words[2];
} ScreenPoint;

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *source);
void xglVectorMulAdd(Vector4 *destination, const Vector4 *source, const Vector4 *scale,
                     const Vector4 *offset);

/* grRotTransPers: transforms the point by the matrix, divides by w, clips to [-1, 1] (flag
 * GR_VERTEX_OUT_OF_FRUSTUM) and maps an in-bounds point to screen X/Y/Z by the camera's screenScale and
 * screenOffset. */
float grRotTransPers(ScreenPoint *out, Matrix *matrix, const Vector4 *point, int *clipFlags)
{
    Vector4 clip;
    StudioCamera *camera;
    float invW;
    int outside;

    xglVectorMulMat(&clip, matrix->elements, point);
    invW = 1.0f / clip.w;
    clip.w = 1.0f;
    clip.x = clip.x * invW;
    clip.y = clip.y * invW;
    clip.z = clip.z * invW;
    outside = 0;
    if (clip.x > 1.0f || clip.x < -1.0f) {
        outside = 1;
    }
    if (clip.y > 1.0f || clip.y < -1.0f) {
        outside++;
    }
    if (clip.z > 1.0f || clip.z < -1.0f) {
        outside++;
    }
    if (outside == 0) {
        xglStudioGetCamera(&camera, 0);
        xglVectorMulAdd(&clip, &clip, &camera->screenScale, &camera->screenOffset);
    } else {
        *clipFlags |= GR_VERTEX_OUT_OF_FRUSTUM;
    }
    out->xyz.z = (int)clip.z;
    out->xyz.y = (int)(clip.y * 16.0f);
    out->xyz.x = (int)(clip.x * 16.0f);
    out->xyz.flags = *clipFlags & GR_VERTEX_OUT_OF_FRUSTUM;
    return invW;
}

extern void *memset(void *destination, int value, unsigned int count);

/* grBBCalc: projects the object origin (0, 0, 0, 1) through grCalcMatrix's matrix. Returns 0 when the point is
 * clipped; otherwise stores its screen X/Y with the GS window offsets GR_GS_WINDOW_OFFSET_X and
 * GR_GS_WINDOW_OFFSET_Y removed and divided by 16. */
int grBBCalc(TransformState *state, int *screenX, int *screenY)
{
    Vector4 origin;
    Matrix matrix;
    ScreenPoint point;
    int clipFlags;

    memset(&origin, 0, sizeof(origin));
    origin.w = 1.0f;
    clipFlags = 0;
    grCalcMatrix(state, &matrix);
    grRotTransPers(&point, &matrix, &origin, &clipFlags);
    if ((clipFlags & GR_VERTEX_OUT_OF_FRUSTUM) != 0) {
        return 0;
    }
    *screenX = (point.xyz.x - GR_GS_WINDOW_OFFSET_X) / 16;
    *screenY = (point.xyz.y - GR_GS_WINDOW_OFFSET_Y) / 16;
    return 1;
}

/* The mover block that slideSetStep and objStdMove move: position, velocity, acceleration and target as
 * Vector4s (w unused). The callers keep it at their +0x10. */
typedef struct StdMover {
    Vector4 position;     /* +0x00 */
    unsigned char unmodeled_10[0x30];
    Vector4 velocity;     /* +0x40 */
    Vector4 acceleration; /* +0x50 */
    Vector4 target;       /* +0x60 */
} StdMover;

/* objStdMove: advances the mover one step toward its target on all three axes and returns 1 when all three
 * have arrived. */
int objStdMove(StdMover *mover)
{
    int arrived;

    arrived = 0;
    {
        float velocity;
        float position;
        float distance;
        float magnitude;

        velocity = mover->velocity.x + mover->acceleration.x;
        position = mover->position.x + velocity;
        mover->velocity.x = velocity;
        mover->position.x = position;
        if (position == mover->target.x) {
            arrived++;
        } else if ((velocity > 0.0f && mover->target.x < position)
                   || (velocity < 0.0f && position < mover->target.x)
                   || ((distance = mover->target.x - position) < 0.0f
                           ? (magnitude = -distance, magnitude <= STD_MOVE_ARRIVE_EPSILON)
                           : (magnitude = distance, magnitude <= STD_MOVE_ARRIVE_EPSILON))) {
            mover->position.x = mover->target.x;
            arrived++;
        }
    }

    {
        float velocity;
        float position;
        float distance;
        float magnitude;

        velocity = mover->velocity.y + mover->acceleration.y;
        position = mover->position.y + velocity;
        mover->velocity.y = velocity;
        mover->position.y = position;
        if (position == mover->target.y) {
            arrived++;
        } else if ((velocity > 0.0f && mover->target.y < position)
                   || (velocity < 0.0f && position < mover->target.y)
                   || ((distance = mover->target.y - position) < 0.0f
                           ? (magnitude = -distance, magnitude <= STD_MOVE_ARRIVE_EPSILON)
                           : (magnitude = distance, magnitude <= STD_MOVE_ARRIVE_EPSILON))) {
            mover->position.y = mover->target.y;
            arrived++;
        }
    }

    {
        float velocity;
        float position;
        float distance;
        float magnitude;

        velocity = mover->velocity.z + mover->acceleration.z;
        position = mover->position.z + velocity;
        mover->velocity.z = velocity;
        mover->position.z = position;
        if (position == mover->target.z) {
            arrived++;
        } else if ((velocity > 0.0f && mover->target.z < position)
                   || (velocity < 0.0f && position < mover->target.z)
                   || ((distance = mover->target.z - position) < 0.0f
                           ? (magnitude = -distance, magnitude <= STD_MOVE_ARRIVE_EPSILON)
                           : (magnitude = distance, magnitude <= STD_MOVE_ARRIVE_EPSILON))) {
            mover->position.z = mover->target.z;
            arrived++;
        }
    }
    return arrived == 3;
}
