#include "common.h"
#include "shared.h"

/* Four-component scale: dst = src * scale (same body as XrgScaleVector in X1 ov12). */
void XrgScaleVector(Vector4 *dst, Vector4 *src, float scale) {
    dst->x = src->x * scale;
    dst->y = src->y * scale;
    dst->z = src->z * scale;
    dst->w = src->w * scale;
}

/* Componentwise product of two vectors, all four components. */
void XrgMultiplyVector(Vector4 *dst, Vector4 *lhs, Vector4 *rhs) {
    dst->x = lhs->x * rhs->x;
    dst->y = lhs->y * rhs->y;
    dst->z = lhs->z * rhs->z;
    dst->w = lhs->w * rhs->w;
}

/* In-place accumulate: dst += src, all four components. */
void XrgAccumulateVector(Vector4 *dst, Vector4 *src) {
    dst->x += src->x;
    dst->y += src->y;
    dst->z += src->z;
    dst->w += src->w;
}
