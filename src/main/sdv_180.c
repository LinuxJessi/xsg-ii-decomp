#include "common.h"
#include "shared.h"

void sdvSetAmbState2(int state, int effect_no);

char *srsAnalyzeEftNo(int effectNo, int *charNo, int *effectType);

extern int _sdvAmbFrame;
extern int _sdvAmbState;
extern int charID_0;
extern int eftCate_1;

void sdvSetAmbStateSub(int state, int effect_no, int force);

extern void *xglStudioGetLight2(void);
extern void xglLightIntensityAmbient(void *light, void *ambient);
extern void func_A2C3D8(void *map_rgb);

extern Vector4 *MMathVectorInterpolation(Vector4 *destination,
                                         const Vector4 *first,
                                         const Vector4 *second,
                                         float parameter);

extern float _sdvMapRgb[4];
extern float _sdvAmbient[4];

void sdvSetAmbient(void *map_rgb, void *ambient);

int scGetImmAdrImmIdx2(int base, unsigned short *table, int index);

#define SCRIPT_ADDRESS_NONE 0xFFFF    /* absent table entry or address */

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", func_00266B50);

int scGetImmAdrImmIdx2(int base, unsigned short *table, int index)
{
    int offset;
    int address;

    offset = table[index];
    if (offset == SCRIPT_ADDRESS_NONE ||offset == -1) {
        offset = 0;
    }
    address = 0;
    if (offset != 0) {
        address = base + offset * 2;
    }
    return address;
}

short scGetImmNumIdx(short *table, int index)
{
    return table[index];
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", func_00266BC8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", func_00266D40);

int scOfsToAdr(int offset)
{
    if (offset == SCRIPT_ADDRESS_NONE ||offset == -1) {
        offset = 0;
    }
    return offset;
}

void sdvInitAmbient(void)
{
    _sdvAmbFrame = 0;
    _sdvAmbState = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", func_00266D88);

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", sdvSetAmbStateSub);

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", func_00266DE8);

void sdvSetAmbState2(int state, int effect_no)
{
    sdvSetAmbStateSub(state, effect_no, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_180", sdvSetAmbient);

void sdvRestoreAmbient(void)
{
    sdvSetAmbient(_sdvMapRgb, _sdvAmbient);
}

void sdvExecAmbient(void)
{
    Vector4 map_rgb;
    Vector4 ambient;
    float parameter;
    int state = _sdvAmbState;

    if (state == 0) {
        return;
    }

    if (state == 1) {
        if (_sdvAmbFrame >= 20) {
            _sdvAmbState = 0;
            _sdvAmbFrame = 20;
            state = 0;
        } else {
            _sdvAmbFrame++;
        }
    } else if (state == 2) {
        if (_sdvAmbFrame <= 0) {
            _sdvAmbFrame = 0;
            state = 0;
            _sdvAmbState = 0;
        } else {
            _sdvAmbFrame--;
        }
    }

    if (state != 0) {
        float half = 0.5f;
        float divisor = 20.0f;

        parameter = (float)_sdvAmbFrame / divisor;
        /* These constrained operations implement the eight evidenced COP2 instructions. */
        __asm__ __volatile__(
            "lqc2 vf1, 0(%1)\n\t"
            "mfc1 $8, %0\n\t"
            "qmtc2 $8, vf2\n\t"
            "vmulx.xyz vf1, vf1, vf2x\n\t"
            "sqc2 vf1, 0(%2)"
            :
            : "f"(half), "r"(&_sdvMapRgb[0]), "r"(&map_rgb)
            : "$8", "memory");
        __asm__ __volatile__(
            "lqc2 vf1, 0(%1)\n\t"
            "mfc1 $8, %0\n\t"
            "qmtc2 $8, vf2\n\t"
            "vmulx.xyz vf1, vf1, vf2x\n\t"
            "sqc2 vf1, 0(%2)"
            :
            : "f"(half), "r"(&_sdvAmbient[0]), "r"(&ambient)
            : "$8", "memory");
        MMathVectorInterpolation(&map_rgb, (const Vector4 *)_sdvMapRgb,
                                 &map_rgb, parameter);
        MMathVectorInterpolation(&ambient, (const Vector4 *)_sdvAmbient,
                                 &ambient, parameter);
        sdvSetAmbient(&map_rgb, &ambient);
    }
}
