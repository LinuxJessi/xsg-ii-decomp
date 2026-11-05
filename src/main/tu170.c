#include "common.h"
#include "shared.h"

typedef float Matrix4[4][4];

#include "quadword.h"
#include "vu0_macro.h"

#define SEF_RAND_MULTIPLIER 0x41C64E6Du
#define SEF_RAND_INCREMENT 0x3039u
#define SEF_RAND_MASK 0x7FFF

void sefGetCubePosBtm(Vector4 *pos, const int *extent);

extern unsigned int sefRandSeed;

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261960);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002619D8);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261A10);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261A38);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261A88);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261AC8);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261B48);

void sefMergeMatrixPos(Matrix4 *dest, Matrix4 *matrix, Vector4 *position)
{
    QuadwordMergeMatrixPosition(dest, matrix, position);
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261BC0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261CE0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261D20);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261D50);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261E40);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261EB0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261EF0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262120);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262208);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262358);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002624B0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262578);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262B40);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262DA8);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262DF8);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262E90);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262F40);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetCubePosBtm);

void sefGetCubePosTop(Vector4 *pos, const int *extent)
{
    sefGetCubePosBtm(pos, extent);
    pos->y = -pos->y;
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263048);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263088);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002632F0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002634C0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263550);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002635E0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263640);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263720);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002639A0);
