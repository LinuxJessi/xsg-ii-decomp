#include "common.h"
#include "shared.h"

unsigned short xglSRand(void);

extern int rnd(int max);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A807E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80800);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80848);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A808B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80918);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80970);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80988);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A809C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A809E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80A20);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80A80);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80AB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80AB8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80B00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80B38);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80B58);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80B78);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80B98);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80BE8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C28);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C30);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C38);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C40);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C58);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80C88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80D20);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80D78);

int rnd(int max)
{
    return xglSRand() % (max + 1);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80DC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu005", func_00A80DF8);
