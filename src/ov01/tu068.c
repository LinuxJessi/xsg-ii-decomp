/*
 * OV01 original TU 68: 0x00aa8c10..0x00aa8ea0 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu001.h"
#include "ov01/tu067.h"

/*
 * Partial layout of the task func_00A802F8 returns (the same task as tu067's
 * ScreenFlashTask): only the words this TU writes are evidenced.  The
 * allocation size is not known here.
 */
struct ScreenFillTask {
    unsigned char unmodeled_00[4];
    int state; /* +0x4, the update slot of the task; zeroed here, so this fill has no per-frame update */
    void (*draw)(void); /* +0x8, set to screenFillGray (grey screen fill) */
};

void *func_00A802F8(void);
/* The running grey screen fill, 0 when none. */
extern struct ScreenFillTask *screenFillTask;

/* Starts a grey screen fill that lasts until screenFillEnd. */
void screenFillStart(void)
{
    struct ScreenFillTask *task = func_00A802F8();

    if (task != 0)
    {
        task->state = 0;
        task->draw = screenFillGray;
        screenFillTask = task;
    }
}

/* Ends the grey screen fill started by screenFillStart. */
void screenFillEnd(void)
{
    if (screenFillTask != 0)
    {
        ov01TaskRelease(screenFillTask);
        screenFillTask = 0;
    }
}

/* Draws a fill of the given RGB colour over the screen at alpha 0x80 (grey in screenFillGray). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu068", grPacketScreenFill);
