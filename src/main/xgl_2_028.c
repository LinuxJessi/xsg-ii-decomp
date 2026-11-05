#include "common.h"
#include "shared.h"

unsigned short xglSRand(void);

extern unsigned long long iRandSeed;

unsigned short xglSRand(void)
{
    iRandSeed = iRandSeed * 1103515245ULL + 12345ULL;
    return (iRandSeed >> 16) & 0x7fff;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_028", func_001A7604);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_028", xglFRand);
