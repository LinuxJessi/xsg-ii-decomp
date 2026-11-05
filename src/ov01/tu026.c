#include "common.h"
#include "shared.h"

extern int calcStatIdx2Bit(int statIdx);
extern int calcStatBit2Idx(int statBit);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BC60);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BCA0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BCD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BD48);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BDD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BED0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BFD0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C0D0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C1C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C1E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C278);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C330);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C3C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C4C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C648);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C778);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8CF88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D110);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D240);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D320);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D380);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D528);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D6E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D890);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D8C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D8F0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D950);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D9C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D9E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DB38);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DB90);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcHpDmg);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DD00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DFF0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E0C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E110);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E3E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E420);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E4A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E4D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E540);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E630);

/*
 * The inverse of calcStatIdx2Bit below: the index of statBit's lowest set bit
 * among the 16 status bits calcStatSet, calcStatReset and calcStatDefTurnGet
 * index with it, or 16 when statBit holds none of them.
 */
int calcStatBit2Idx(int statBit)
{
    int statIdx;
    int mask;

    mask = 1;
    for (statIdx = 0; statIdx < 16; statIdx++) {
        if (statBit & mask) {
            break;
        }
        mask <<= 1;
    }
    return statIdx;
}

int calcStatIdx2Bit(int statIdx)
{
    return 1 << statIdx;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E6E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E748);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E808);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E9B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8ED88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8EEE0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8EF40);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8EF80);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8F070);
