#include "common.h"
#include "shared.h"

/* Fade record: color is the value fadeSetColor/fadeReset set directly; current is what
 * fadeUpdate sends to xglRenderGlobalFadeSet every frame; start/target/delta drive the
 * per-frame interpolation (delta = (target - start) / totalFrames, added to current
 * each frame while active and skipAnimate is 0). */
typedef struct FadeState {
    int active;         /* +0x00 */
    Vector4 start;       /* +0x04 */
    Vector4 current;      /* +0x14 */
    Vector4 target;       /* +0x24 */
    Vector4 color;        /* +0x34 */
    Vector4 delta;         /* +0x44 */
    int elapsed;         /* +0x54 */
    int totalFrames;     /* +0x58 */
    int skipAnimate;     /* +0x5C */
} FadeState;

extern FadeState fadeState;
extern int fadeUpdateHook[];
void *memset(void *destination, int value, unsigned int count);
void fadeApplyColor(float red, float green, float blue, float alpha);
void xglRenderGlobalFadeReset(void);

/* Resets the fade: black, fully opaque, no fade in progress. */
void fadeReset(void)
{
    memset(&fadeState, 0, sizeof(fadeState));
    fadeState.color.x = 0.0f;
    fadeState.color.y = 0.0f;
    fadeState.color.z = 0.0f;
    fadeState.color.w = 255.0f;
    fadeState.current = fadeState.color;
    xglRenderGlobalFadeReset();
    fadeApplyColor(fadeState.color.x, fadeState.color.y, fadeState.color.z, fadeState.color.w);
    fadeUpdateHook[0] = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeSetColor);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartIn);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartOut);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartColor);

INCLUDE_ASM("asm/main/nonmatchings/tu198", fadeStartColorAlpha);

void fadeStartIn(int frames);
void xglRenderGlobalFadeSet(float x, float y, float z, float w);
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
