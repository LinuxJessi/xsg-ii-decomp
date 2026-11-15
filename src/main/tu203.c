#include "common.h"
#include "shared.h"

int func_00274100(int value)
{
    int magnitude = (value >= 0) ? value : -value;

    if (value > 0) {
        return (value << 4) + 8;
    }
    return -(magnitude << 4) - 8;
}

INCLUDE_ASM("asm/main/nonmatchings/tu203", func_00274130);

extern int xglCosDegTableReady;
extern float xglCosDegTable[];
void xglCosDegTableInit(void);

/* Cosine of an angle in degrees from the 10-degree table (built on first use). */
float xglCosDeg(int degrees)
{
    if (!xglCosDegTableReady) {
        xglCosDegTableInit();
        xglCosDegTableReady = 1;
    }
    /* |degrees| taken in place: with a separate local gcc leaves the abs after the init call (bgez/negu). */
    degrees = (degrees >= 0) ? degrees : -degrees;
    return xglCosDegTable[(degrees % 360) / 10];
}

extern float xglDegToRad;
extern float cosf(float angle);

void xglCosDegTableInit(void)
{
    float degToRad = xglDegToRad;
    int i;

    for (i = 0; i < 37; i++) {
        xglCosDegTable[i] = cosf((float) (i * 10) * degToRad);
    }
}

extern int xglSinDegTableReady;
extern float xglSinDegTable[];
void xglSinDegTableInit(void);

/* Sine of an angle in degrees from the 10-degree table (built on first use). */
float xglSinDeg(int degrees)
{
    if (!xglSinDegTableReady) {
        xglSinDegTableInit();
        xglSinDegTableReady = 1;
    }
    /* |degrees| taken in place, as in xglCosDeg. */
    degrees = (degrees >= 0) ? degrees : -degrees;
    return xglSinDegTable[(degrees % 360) / 10];
}

extern float D_0069B568;
extern float sinf(float angle);

void xglSinDegTableInit(void)
{
    float degToRad = D_0069B568;
    int i;

    for (i = 0; i < 37; i++) {
        xglSinDegTable[i] = sinf((float) (i * 10) * degToRad);
    }
}
