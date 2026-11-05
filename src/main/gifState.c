#include "common.h"
#include "shared.h"

void nmlModelUseSubWindow(unsigned int index, int enabled);

#define NML_WINDOW_COUNT 4

extern int g_aSubWindow[NML_WINDOW_COUNT];

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00241A20);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00241CD8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002424D8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00242C98);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00242F90);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00243448);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002439C8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002439D0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00243B58);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00243BA0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00243D50);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00244938);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00245728);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00245A50);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246AF8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246B20);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246B50);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246BD8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246C88);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246CE8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246D08);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246D30);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246D60);

INCLUDE_ASM("asm/main/nonmatchings/gifState", nmlModelSetMulColor);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246D98);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246DB0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246DC0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246DD8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246DE8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246DF8);

void nmlModelUseSubWindow(unsigned int index, int enabled) {
    if (index < NML_WINDOW_COUNT) {
        g_aSubWindow[index] = enabled;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246E30);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246E58);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246EA0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246EB0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246ED8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246F60);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246F80);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246FE0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246FF8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002470D0);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002470E0);
