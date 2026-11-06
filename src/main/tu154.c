#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu154", MMathEvalHermiteKeys);

void func_00252838(void)
{
}

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252840);

float MMathCalcDistXZ(int, int);

/* Returns 1 when the XZ distance between the points at +0x10 of the two entries is within limit, 0 otherwise. */
int MMathIsWithinDistXZ(int firstEntry, int secondEntry, float limit)
{
    if (MMathCalcDistXZ(firstEntry + 0x10, secondEntry + 0x10) <= limit) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252980);

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252AB8);

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252CD8);
