#include "common.h"
#include "shared.h"

extern int padMode;
extern short padData;

#define VIRTUAL_PAD_BIT_COUNT 14

extern unsigned short padConvTbl[VIRTUAL_PAD_BIT_COUNT];

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A88E70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A88E98);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A88EB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89038);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A891C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89268);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89280);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89340);

void dataVPadModeSet(int mode)
{
    padMode = mode;
    padData = 0;
}

void dataVPadSet(int virtualPad)
{
    unsigned short result = 0;
    int shift = 0;
    const unsigned short *entry = padConvTbl;
    virtualPad &= 0xFFFF;
    /* Original entry at 0x00A1D2A0 enters the body before testing the
     * loop count; keep the table pointer advance and 16-bit mask. */
    do {
        if ((virtualPad >> shift) & 1) {
            result |= *entry;
        }
        shift++;
        entry++;
    } while (shift < VIRTUAL_PAD_BIT_COUNT);
    padData = result;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", dataPadRead);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89520);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89538);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89580);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89710);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89888);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89898);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A898A8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A898B8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89998);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A899A8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A899D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89A08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89AC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89B00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89C08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89C60);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89CB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89D68);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89DF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89E70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89E90);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89EA8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89F08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A018);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A110);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A150);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A160);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A1E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A2E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A3B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A490);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A538);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A730);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A878);

void dataDummySet(void)
{
}
