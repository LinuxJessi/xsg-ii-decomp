#include "common.h"
#include "main/control_entry.h"
#include "main/xgl_render.h"
#include "main/xgl_studio.h"

INCLUDE_ASM("asm/main/nonmatchings/control_entry", InitializeSystem);

/*
 * Entry point: stores argc and argv in mainParamArgc and mainParamArgv, runs
 * InitializeSystem, then loops on the title screen selection.
 */
INCLUDE_ASM("asm/main/nonmatchings/control_entry", gameMain);

#include "shared.h"

void xglCdControlThread(void);

void xglSleep(void) {
    xglRenderFrame();
    xglCdControlThread();
    xglStudioUpdate();
}

/*
 * EE Timer 0.  T0_COUNT is the free-running counter and T0_MODE its control
 * word; bit 7 (0x80) of T0_MODE is CUE, which starts the count.
 */
#define T0_COUNT ((volatile u32 *)0x10000000)
#define T0_MODE ((volatile u32 *)0x10000010)
#define T0_MODE_CUE 0x80

void xglTimer0Reset(int mode) {
    *T0_COUNT = 0;
    *T0_MODE = mode + T0_MODE_CUE;
}
