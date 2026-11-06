#include "common.h"
#include "shared.h"
#include "main/xgl_2_028.h"


extern unsigned long long iRandSeed;

unsigned short xglSRand(void)
{
    iRandSeed = iRandSeed * 1103515245ULL + 12345ULL;
    return (iRandSeed >> 16) & 0x7fff;
}

/* Two steps of the iRandSeed generator; returns the first step shifted left 16 plus the second state shifted right 16. */
INCLUDE_ASM("asm/main/nonmatchings/xgl_2_028", xglLRand);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_028", xglFRand);
