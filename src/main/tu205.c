#include "common.h"
#include "shared.h"

extern u16 sRenderDisplayBase;
extern u16 sRenderFrameBase;
extern u16 D_00645842;

/* GS FRAME register value of a 512-pixel-wide (FBW 8) frame page: buffer 0 the display page, 1 the page D_00645842,
   2 sRenderFrameBase; flag is not read. */
u64 gsMakeFrame(int buffer, int flag)
{
    switch (buffer) {
    case 0:
        return sRenderDisplayBase | 0x80000;
    case 1:
        return D_00645842 | 0x80000;
    case 2:
        return sRenderFrameBase | 0x80000;
    default:
        return 0;
    }
}

extern void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

/* Normalizes v in place (through a copy of it). */
void xglVectorNormalInPlace(Vector4 *v)
{
    Vector4 source = *v;

    xglVectorNormal_001A9200(v, &source);
}

/* GS ALPHA register value for a blend mode: 1 additive (Cs*As + Cd), 2 subtractive
 * (-Cs*As + Cd), 3 (Cs - Cd)*As with no destination term, anything else normal
 * alpha blending ((Cs - Cd)*As + Cd). */
int gsMakeAlpha(int blend)
{
    if (blend == 1) {
        return 0x48;
    }
    if (blend >= 2) {
        if (blend == 2) {
            return 0x42;
        }
        if (blend == 3) {
            return 0x84;
        }
    }
    return 0x44;
}
