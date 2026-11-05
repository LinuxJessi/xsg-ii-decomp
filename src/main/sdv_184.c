#include "common.h"
#include "shared.h"

void sdvClearSpecialWork(void);

extern short _sdvSpecialBuf[8];
extern void GameDefocusSet(int index, int, const int *);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00268BF8);

void sdvClearSpecialWork(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (_sdvSpecialBuf[i] != 0) {
            GameDefocusSet(i, 0, 0);
            _sdvSpecialBuf[i] = 0;
        }
    }
}

int sdvAllocSpecialWork(int kind)
{
    int slot;

    if (kind == 0) {
        if (_sdvSpecialBuf[0] == 0) {
            _sdvSpecialBuf[0] = 1;
            return 0;
        }
        return -1;
    }

    for (slot = 1; slot < 8; slot++) {
        if (_sdvSpecialBuf[slot] == 0) {
            _sdvSpecialBuf[slot] = 1;
            return slot;
        }
    }
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00268D18);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00268D60);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00268DA8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00268EB8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00269508);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_00269FB8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A318);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A330);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A6A0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A6B8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A718);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A7A0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026AA08);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026AF68);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026AFB0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026B070);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026B9B8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026BA08);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026BA58);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvDrawAlters);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026BB78);
