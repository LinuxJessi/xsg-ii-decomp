#include "common.h"

#include "shared.h"
#include "main/tu204.h"

/* Per-channel lerp of two packed 8-bit colours by t: the four channels are masked with 0xFF, 0xFF00, 0xFF0000 and
 * 0xFF000000 and each one is moved from its from value towards its to value. */
long long xglColorLerp(long long from, long long to, float t)
{
    long long fromRed = from & 0xFF;
    long long fromGreen = from & 0xFF00;
    long long fromBlue = from & 0xFF0000;
    long long fromAlpha = from & 0xFF000000;
    long long toRed = to & 0xFF;
    long long toGreen = to & 0xFF00;
    long long toBlue = to & 0xFF0000;
    long long toAlpha = to & 0xFF000000;
    long long red;
    long long green;
    long long blue;
    long long alpha;

    red = (u32)((float) fromRed + (float) (toRed - fromRed) * t) & 0xFF;
    green = (u32)((float) fromGreen + (float) (toGreen - fromGreen) * t) & 0xFF00;
    blue = (u32)((float) fromBlue + (float) (toBlue - fromBlue) * t) & 0xFF0000;
    alpha = (u32)((float) fromAlpha + (float) (toAlpha - fromAlpha) * t) & 0xFF000000;
    return red | green | blue | alpha;
}

/* Wraps an angle in degrees into [0, 360]: one step of 360 each way. */
float xglWrapDegrees(float degrees) {
    if (degrees > 360.0f) {
        degrees -= 360.0f;
    }
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

#include "gs.h"
#include "main/tu093.h"

extern u16 D_00645842;
extern u16 sRenderFrameBase;

/* GS TEX0 of a 512x512 PSMCT32 frame page (TBW 8, TW and TH 9): buffer 0 is the display page with CBP set to
 * frame_base_1, buffer 1 the page D_00645842, buffer 2 sRenderFrameBase; tex_cc goes to TCC, TFX stays 0. */
u64 gsMakeTex0(int buffer, int tex_cc)
{
    switch (buffer) {
    case 0:
        return GS_SET_TEX0(sRender.display_base << 5, 8, GS_PSMCT32, 9, 9, tex_cc, 0, sRender.frame_base_1, 0, 0, 0, 0);
    case 1:
        return GS_SET_TEX0(D_00645842 << 5, 8, GS_PSMCT32, 9, 9, tex_cc, 0, 0, 0, 0, 0, 0);
    case 2:
        return GS_SET_TEX0(sRenderFrameBase << 5, 8, GS_PSMCT32, 9, 9, tex_cc, 0, 0, 0, 0, 0, 0);
    default:
        return 0;
    }
}
