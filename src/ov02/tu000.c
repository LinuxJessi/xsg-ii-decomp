#include "common.h"
#include "shared.h"

extern void xglSleep(void);
extern void xglFontPrintDirectOT(int ot, const char *text);

int xglMcMain(void);

/* Defined here: deviceCheckFontModeText, TestEnv_1_00A0F690, namcoLogoPath, msiLogoPath (data order is section order). */
extern const char deviceCheckFontModeText[8];

int device_check_sync(void)
{
    int status;

    status = xglMcMain();
    xglSleep();
    xglFontPrintDirectOT(-1, deviceCheckFontModeText);
    return status;
}

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80040);

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80048);

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A803B8);

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80410);

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80590);

INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80790);
