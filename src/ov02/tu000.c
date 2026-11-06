#include "common.h"
#include "shared.h"
#include "main/control_entry.h"
#include "main/xgl_font.h"

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

/* Card-check gate of the logo screen: always 1 in this build. Left unnamed. */
int func_00A80040(void) {
    return 1;
}

/* Memory-card check (proposed logoMcCheck): polls both slots and shows the save-space messages. */
INCLUDE_ASM("asm/nonmatchings/ov02/tu000", logoMcCheck);

/* Logo screen entry (proposed LogoFirst): clears the frame, runs the card check, then the credit roll. */
INCLUDE_ASM("asm/nonmatchings/ov02/tu000", LogoFirst);

/* One band of the credit image (proposed logoDrawCreditBand, low confidence). */
INCLUDE_ASM("asm/nonmatchings/ov02/tu000", func_00A80410);

/* Credit roll (proposed logoShowCredits): loads credit.bin and draws it in 0x80-row bands. */
INCLUDE_ASM("asm/nonmatchings/ov02/tu000", logoShowCredits);

void func_00A80790(void) {

}
