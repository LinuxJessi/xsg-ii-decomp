#include "common.h"
#include "main/control_entry.h"
#include "main/xgl_render.h"

INCLUDE_ASM("asm/main/nonmatchings/tu127", func_0023C600);

INCLUDE_ASM("asm/main/nonmatchings/tu127", xglTexUploadList);

INCLUDE_ASM("asm/main/nonmatchings/tu127", func_0023CB88);

INCLUDE_ASM("asm/main/nonmatchings/tu127", func_0023CC80);

INCLUDE_ASM("asm/main/nonmatchings/tu127", func_0023CCF8);


/* Clears the frame, then sleeps forever: a test loop that never returns. */
void ItaiTest(void) {
    xglRenderClearFrame();
    for (;;) {
        xglSleep();
    }
}
