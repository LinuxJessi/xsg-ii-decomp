#include "common.h"
#include "shared.h"
#include "main/tu079.h"

void func_A9CDB0(void *, void *);
typedef struct MathNativeEntry {
    const char *name;
    int (*function)();
} MathNativeEntry;

extern MathNativeEntry mathNativeTable[5];
extern char D_0069D3B8[8];

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

/* Native table: (name, function) pairs, then a null entry. Passed to func_A9CDB0 by mathInit. */
extern unsigned char D_0069D3B0[];
extern unsigned char D_0069D3A8[];
extern unsigned char D_0069D3A0[];
extern unsigned char D_0069D398[];
int mathAtan();
int math_001FC398();
int math_001FC438();
int math_001FC4D8();

MathNativeEntry mathNativeTable[5] = {
    { D_0069D3B0, mathAtan },
    { D_0069D3A8, math_001FC398 },
    { D_0069D3A0, math_001FC438 },
    { D_0069D398, math_001FC4D8 },
    { 0, 0 },
};

char D_0069D3B8[8] = "math";
char D_0069D3C0[8] = "ptTent";
char D_0069D3C8[8] = "ptSet";
char D_0069D3D0[8] = "ptLeave";
char D_0069D3D8[8] = "ptJoin";
char D_0069D3E0[8] = "ptClear";
char D_0069D3E8[8] = "ptInit";
