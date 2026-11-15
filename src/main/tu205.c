#include "common.h"
#include "shared.h"

extern u16 sRenderDisplayBase;
extern u16 sRenderFrameBase;
extern u16 D_00645842;

u64 func_00274650(int buffer, int flag)
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

void func_002746B8(Vector4 *v)
{
    Vector4 tmp = *v;

    xglVectorNormal_001A9200(v, &tmp);
}

/* GS ALPHA register value for a blend mode: 1 additive (Cs*As + Cd), 2 subtractive
 * (-Cs*As + Cd), 3 (Cs - Cd)*As with no destination term, anything else normal
 * alpha blending ((Cs - Cd)*As + Cd). */
int func_002746F8(int blend)
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
