#include "common.h"

/* 0x00274368: per-channel lerp of two packed 8-bit colours by the float t in $f12
 * (proposed name xglColorLerp); the four channels are masked with 0xFF, 0xFF00, 0xFF0000, 0xFF000000. */
INCLUDE_ASM("asm/main/nonmatchings/tu204", xglColorLerp);

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

/* 0x002745A8: returns a 64-bit register word for mode 0, 1 or 2; mode 0 reads sRender +0x20 and +0x1A,
 * modes 1 and 2 read the halfwords D_00645842 and sRenderFrameBase. Role not yet proven, no name proposed. */
INCLUDE_ASM("asm/main/nonmatchings/tu204", func_002745A8);
