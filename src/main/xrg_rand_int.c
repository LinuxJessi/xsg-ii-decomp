#include "common.h"
#include "shared.h"

typedef float RgVector[4];

void XrgOuterVector(RgVector destination, RgVector first, RgVector second);

float XrgInnerVector(RgVector first, RgVector second);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296688);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296770);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296878);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296B20);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296BD8);

float XrgInnerVector(RgVector first, RgVector second)
{
    return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
}

void XrgOuterVector(RgVector destination, RgVector first, RgVector second)
{
    destination[0] = first[1] * second[2] - first[2] * second[1];
    destination[1] = first[2] * second[0] - first[0] * second[2];
    destination[2] = first[0] * second[1] - first[1] * second[0];
}

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296CF0);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296E38);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_00296F28);

INCLUDE_ASM("asm/main/nonmatchings/xrg_rand_int", func_002970E8);
