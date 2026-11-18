#include "common.h"
#include "shared.h"
#include "main/tu079.h"

void func_A9CDB0(void *, void *);
typedef struct MathNativeEntry {
    const char *name;
    int (*function)();
} MathNativeEntry;

/* Native table: (name, function) pairs, then a null entry. Passed to func_A9CDB0 by mathInit; its names come before "math" in .sdata. */
int mathAtan();
int math_001FC398();
int math_001FC438();
int math_001FC4D8();

MathNativeEntry mathNativeTable[5] = {
    { "atan", mathAtan },
    { "sin", math_001FC398 },
    { "cos", math_001FC438 },
    { "random", math_001FC4D8 },
    { 0, 0 },
};

/* Registers the math native table mathNativeTable (atan, sin, cos, random) under the module name "math". */
void mathInit(void)
{
    func_A9CDB0("math", mathNativeTable);
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


