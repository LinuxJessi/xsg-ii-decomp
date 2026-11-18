/*
 * OV01 original TU 39: 0x00a9af68..0x00a9b0f8 (5 functions)
 *
 * Battle step chain (the continuation set by ov01StepSetNext): ov01StepBattle ->
 * ov01StepStartFadeOut -> ov01StepAwaitObjectsIdle -> ov01StepBeginFinish -> ov01StepFinish.
 */
#include "common.h"
#include "ov01/tu035.h"
#include "ov01/tu036.h"
#include "ov01/tu037.h"
#include "ov01/tu039.h"
#include "ov01/tu052.h"
#include "ov01/tu064.h"
#include "main/tu048.h"
#include "main/tu148.h"

extern int battleEndCheck(void);
extern void screenFadeOutStart(int a, int b);
extern void ov01CallbackSlotRelease();
void ov01StepSetNext(void *item, void (*next_step)(int context));
extern int ov01SceneDone;
void ov01StepStartFadeOut(int context);

#define FRAMES_PER_SECOND 30

/* First battle step: runs the target selection until it is done, then hides the cursor and moves on to
 * ov01StepStartFadeOut. Under a time limit it shows the remaining time and counts it down by one frame. */
void ov01StepBattle(int context)
{
    int rate; /* the divisor sits in a register before the test, as in the original */

    if (tgtSelectStep() != 0) {
        menuCursorHide();
        ov01StepSetNext(context, ov01StepStartFadeOut);
    }
    rate = FRAMES_PER_SECOND;
    if (cfEventGet() & CF_EVENT_TIME_LIMIT) {
        msgPanelSetTimecode(0, 0, gameWork.panelValue / rate, gameWork.panelValue % rate);
        if (gameWork.panelValue > 0) {
            gameWork.panelValue--;
        }
    }
}

void ov01StepAwaitObjectsIdle(int context);
void ov01StepBeginFinish(int context);
void ov01StepFinish(int context);

/* Battle step: waits until battleEndCheck reports the battle over, then opens a
 * panel object (screenFadeOutStart) and moves on to ov01StepAwaitObjectsIdle. */
void ov01StepStartFadeOut(int context)
{
    if (battleEndCheck() == 0) {
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
void ov01StepFinish(int context)
{
    ov01CallbackSlotRelease();
    ov01SceneDone = 1;
}
