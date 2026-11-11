#include "common.h"
#include "shared.h"
#include "main/tu079.h"

void func_A9CDB0(void *, void *);
extern unsigned char mathNativeTable[];
extern unsigned char D_0069D3B8[];

/* Registers the math native table mathNativeTable (atan, sin, cos, random) under the module descriptor D_0069D3B8 ("math"). */
void mathInit(void)
{
    func_A9CDB0(D_0069D3B8, mathNativeTable);
}

int func_A9F0A8(void);
void func_A9CC70(int, int);
void selectionInit(void);

int ptInit(int context)
{
    if (func_A9F0A8() != 0)
    {
        func_A9CC70(context, 2);
        return 0;
    }
    selectionInit();
    return 1;
}

void ptResetSlots(void);

/* Script native "ptClear": takes no arguments; calls ptResetSlots and returns 1. */
int ptClear(int context)
{
    if (func_A9F0A8() != 0)
    {
        func_A9CC70(context, 2);
        return 0;
    }
    ptResetSlots();
    return 1;
}
