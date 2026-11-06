/*
 * OV01 original TU 39: 0x00a9af68..0x00a9b0f8 (5 functions)
 *
 * Battle step chain (the continuation set by ov01StepSetNext): func_00A9AF68 ->
 * ov01StepStartFadeOut -> ov01StepAwaitObjectsIdle -> ov01StepBeginFinish -> ov01StepFinish.
 */
#include "common.h"
#include "ov01/tu037.h"
#include "ov01/tu064.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu039", func_00A9AF68);

extern int func_00A97288(void);
extern int cfEventGet(void);
extern void screenFadeOutStart(int a, int b);
extern void ov01CallbackSlotRelease();
extern void ov01StepSetNext(void *item, int next_step);
extern int ov01SceneDone;
void ov01StepStartFadeOut(int context);
void ov01StepAwaitObjectsIdle(int context);
void ov01StepBeginFinish(int context);
void ov01StepFinish(void);

/* Battle step: waits for the menu phase (func_00A97288) to finish, then opens a
 * panel object (screenFadeOutStart) and moves on to ov01StepAwaitObjectsIdle. */
void ov01StepStartFadeOut(int context)
{
    if (func_00A97288() == 0) {
        return;
    }
    cfEventGet();
    screenFadeOutStart(4, 2);
    ov01StepSetNext(context, ov01StepAwaitObjectsIdle);
}

/* Battle step: waits while the object list of tu064 is busy (screenFadeIdle is
 * 1 when screenFadeTask is empty), then clears the scene (ov01SceneRelease). */
void ov01StepAwaitObjectsIdle(int context)
{
    if (screenFadeIdle() == 0) {
        return;
    }
    ov01SceneRelease();
    ov01StepSetNext(context, ov01StepBeginFinish);
}

void ov01StepBeginFinish(int context)
{
    ov01StepSetNext(context, ov01StepFinish);
}

/* Last step of the scene: releases its entry and sets ov01SceneDone, the scene
 * done flag that the frame loop (ov01SceneIsDone) polls. */
void ov01StepFinish(void)
{
    ov01CallbackSlotRelease();
    ov01SceneDone = 1;
}
