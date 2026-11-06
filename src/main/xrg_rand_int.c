#include "common.h"
#include "shared.h"

typedef float RgVector[4];

void XrgOuterVector(RgVector destination, RgVector first, RgVector second);
float XrgInnerVector(RgVector first, RgVector second);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", XrgQuaternionNormalize);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", XrgEulerToQuaternion);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", XrgQuaternionToEuler);

f32 func_002A1DC8(f32);
f32 sinf(f32);

/* Axis-angle from a quaternion: angle is twice the value func_002A1DC8 returns for w (an inverse cosine,
 * presumably acosf); the axis is xyz divided by sin(angle / 2), with a zero sine clamped to 1e-6. */
void XrgQuaternionToAxisAngle(Vector4 *axis_angle, Vector4 *quaternion)
{
    f32 angle;
    f32 divisor;

    angle = 2.0f * func_002A1DC8(quaternion->w);
    divisor = sinf(angle * 0.5f);
    if (divisor == 0.0f)
    {
        divisor = 0.000001f;
    }
    axis_angle->x = quaternion->x / divisor;
    axis_angle->y = quaternion->y / divisor;
    axis_angle->z = quaternion->z / divisor;
    axis_angle->w = angle;
}

f32 cosf(f32);

/* Quaternion from axis-angle: w = cos(angle / 2), xyz = axis * sin(angle / 2). */
void XrgAxisAngleToQuaternion(Vector4 *quaternion, Vector4 *axis_angle)
{
    f32 half_angle;
    f32 sin_half_angle;

    half_angle = axis_angle->w * 0.5f;
    quaternion->w = cosf(half_angle);
    sin_half_angle = sinf(half_angle);
    quaternion->x = axis_angle->x * sin_half_angle;
    quaternion->y = axis_angle->y * sin_half_angle;
    quaternion->z = axis_angle->z * sin_half_angle;
}

float XrgInnerVector(RgVector first, RgVector second)
{
    return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
}

void XrgOuterVector(RgVector destination, RgVector first, RgVector second)
{
    destination[0] = first[1] * second[2] - first[2] * second[1];
    destination[1] = first[2] * second[0] - first[0] * second[2];
    destination[2] = first[0] * second[1] - first[1] * second[0];
}

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296CF0);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", XrgQuaternionMultiply);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", XrgQuaternionSlerp);

void XrgQuaternionNormalize(Vector4 *destination, Vector4 *source);

/* Normalised linear interpolation: result = from + blend * (to - from) on all four components,
 * then renormalised by XrgQuaternionNormalize. */
void XrgQuaternionLerpNormalized(Vector4 *result, Vector4 *from, Vector4 *to, f32 blend)
{
    result->x = from->x + blend * (to->x - from->x);
    result->y = from->y + blend * (to->y - from->y);
    result->z = from->z + blend * (to->z - from->z);
    result->w = from->w + blend * (to->w - from->w);
    XrgQuaternionNormalize(result, result);
}
