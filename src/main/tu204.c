#include "common.h"

#include "shared.h"

float func_002A6818(long value);

/* Per-channel lerp of two packed 8-bit colours by t; the four channels are masked with 0xFF, 0xFF00,
 * 0xFF0000 and 0xFF000000 and converted with func_002A6818 (64-bit integer to float). */
u64 xglColorLerp(u64 from, u64 to, float t)
{
    u64 fromRed = from & 0xFF;
    u64 fromGreen = from & 0xFF00;
    u64 fromBlue = from & 0xFF0000;
    u64 fromAlpha = from & 0xFF000000;
    u64 toRed = to & 0xFF;
    u64 toGreen = to & 0xFF00;
    u64 toBlue = to & 0xFF0000;
    u64 toAlpha = to & 0xFF000000;
    u64 red;
    u64 green;
    u64 blue;
    u64 alpha;

    red = (u32)(func_002A6818(fromRed) + func_002A6818(toRed - fromRed) * t) & 0xFF;
    green = (u32)(func_002A6818(fromGreen) + func_002A6818(toGreen - fromGreen) * t) & 0xFF00;
    blue = (u32)(func_002A6818(fromBlue) + func_002A6818(toBlue - fromBlue) * t) & 0xFF0000;
    alpha = (u32)(func_002A6818(fromAlpha) + func_002A6818(toAlpha - fromAlpha) * t) & 0xFF000000;
    return red | green | blue | alpha;
}

/* Wraps an angle in degrees into [0, 360]: one step of 360 each way (proposed name xglWrapDegrees). */
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
