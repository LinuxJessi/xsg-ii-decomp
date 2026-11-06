/*
 * OV01 original TU 64: 0x00aa8590..0x00aa8678 (3 functions)
 */
#include "common.h"
#include "ov01/tu064.h"

/*
 * Starts a screen fade whose alpha rises from 0 to 0x80 (opaque): allocates a
 * task, stores step and delay in it and makes it the running fade.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu064", screenFadeInStart);

/* Starts a screen fade whose alpha falls from 0x80 to 0. Same layout as the rising fade. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu064", screenFadeOutStart);

/* The running screen fade (struct ScreenFade in tu065), 0 when none; reset by menuPanelRowsReset of tu049. */
extern int screenFadeTask;

/* Nonzero when no screen fade is running. */
int screenFadeIdle(void) {
    return screenFadeTask == 0;
}
