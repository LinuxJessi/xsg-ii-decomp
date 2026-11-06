#include "common.h"

extern unsigned char D_006AF4C0[];
extern unsigned char D_006AF640[];

/*
 * Three 0x180-byte blocks (D_006AF4C0, D_006AF640 and D_006AF340) are moved
 * by func_00252F98 and func_00253048. The getters below return two of them;
 * func_00264EE8 (main/tu175) calls both, func_00263720 (main/tu170) the first.
 */
void *func_00252F78(void)
{
    return D_006AF4C0;
}

void *func_00252F88(void)
{
    return D_006AF640;
}

INCLUDE_ASM("asm/main/nonmatchings/tu156", func_00252F98);

INCLUDE_ASM("asm/main/nonmatchings/tu156", func_00253048);
