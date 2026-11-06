#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeReset);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeSetColor);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartIn);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartOut);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartColor);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartColorAlpha);

#include "shared.h"

/* Fade record fadeState and per-frame hook fadeUpdateHook: see fadeUpdate. */
void fadeStartIn(int frames);
void xglRenderGlobalFadeSet(float x, float y, float z, float w);
extern int fadeUpdateHook[];
extern int D_0069DC88;

/* Ends any fade: a zero-frame start, the update hook cleared, transparent black. */
void fadeClear(void)
{
    fadeStartIn(0);
    fadeUpdateHook[0] = 0;
    xglRenderGlobalFadeSet(0.0f, 0.0f, 0.0f, 0.0f);
}

/* dst = minuend - subtrahend, component-wise. */
void fadeSubVector(Vector4 *dst, const Vector4 *minuend, const Vector4 *subtrahend)
{
    dst->x = minuend->x - subtrahend->x;
    dst->y = minuend->y - subtrahend->y;
    dst->z = minuend->z - subtrahend->z;
    dst->w = minuend->w - subtrahend->w;
}

/* dst = (minuend - subtrahend) / divisor; a divisor of 0 leaves the difference. */
void fadeDeltaVector(Vector4 *dst, const Vector4 *minuend, const Vector4 *subtrahend, int divisor)
{
    float scale;

    fadeSubVector(dst, minuend, subtrahend);
    if (divisor != 0) {
        scale = (float) divisor;
        dst->x = dst->x / scale;
        dst->y = dst->y / scale;
        dst->z = dst->z / scale;
        dst->w = dst->w / scale;
    }
}

/* Sends a colour given in 0..255 units to xglRenderGlobalFadeSet as 0..1 floats, unless D_0069DC88 is set. */
void fadeApplyColor(float red, float green, float blue, float alpha)
{
    if (D_0069DC88 == 0) {
        xglRenderGlobalFadeSet(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
    }
}
