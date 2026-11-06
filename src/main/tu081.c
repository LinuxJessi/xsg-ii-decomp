#include "common.h"
#include "main/tu108.h"

INCLUDE_ASM("asm/main/nonmatchings/tu081", ptSetPilot);

INCLUDE_ASM("asm/main/nonmatchings/tu081", menu_001FC9B8);

int func_A9CC70(int value, int mode);
int func_A9F0A8(void);

/* Script native "umnReturn": takes no arguments; calls uwamonoDetachCurrent and returns 1. */
int umnReturn(int context)
{
    if (func_A9F0A8() != 0) {
        func_A9CC70(context, 2);
        return 0;
    }
    uwamonoDetachCurrent();
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu081", ptSetArray);

INCLUDE_ASM("asm/main/nonmatchings/tu081", ptTent);
