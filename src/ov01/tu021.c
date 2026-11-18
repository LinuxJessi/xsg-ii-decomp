/*
 * OV01 original TU 21: 0x00a88cb0..0x00a88e70 (4 functions)
 */
#include "common.h"
#include "ov01/tu001.h"
#include "main/tu051.h"
#include "ov01/tu010.h"
#include "shared.h"

typedef struct FadeTask FadeTask;

typedef struct FadeOwner {
    unsigned char unmodeled_00[0x90];
    struct XenoActor *target;  /* +0x90 */
    FadeTask *fadeActive;   /* +0x94: the running fade task (fadeInStart and
                             * fadeOutStart store it); cleared to 0 when the
                             * fade ends. */
} FadeOwner;

/*
 * Task record of a fade. The fields from +0xC on are the payload of the task
 * record, a union because each kind of task (tu067's screen flash keeps a
 * frame counter there) reads it its own way; only the fade view is modeled.
 */
struct FadeTask {
    unsigned char unmodeled_00[4];
    void (*update)(FadeTask *task);  /* +0x4: fadeInStep or fadeOutStep */
    void (*draw)(void);              /* +0x8: no draw callback */
    union {
        struct {
            FadeOwner *owner;   /* +0xC */
            f32 progress;       /* +0x10: advanced by step each frame */
            f32 step;           /* +0x14 */
        } fade;
    } payload;
};

void fadeInStep(FadeTask *task);
void fadeOutStep(FadeTask *task);

/*
 * fadeInStart: starts a fade-in of the owner's target. It takes a FadeTask
 * at progress 0 with step 0.1, sets the 0x10000 flag on the target, and
 * stores the task in owner+0x94. It returns the task, or 0 when none was free.
 */
FadeTask *fadeInStart(FadeOwner *owner) {
    FadeTask *task = ov01TaskClaim();

    if (task != 0) {
        task->payload.fade.owner = owner;
        task->payload.fade.step = FADE_DEFAULT_STEP;
        task->payload.fade.progress = 0.0f;
        task->update = fadeInStep;
        task->draw = 0;
        owner->fadeActive = task;
        owner->target->drawFlags |= ACTOR_STATE_FADING;
    }
    return task;
}

/*
 * fadeInStep: the per-frame step of the fade-in
 * task. At 1.0 it clears the target's 0x10000 flag, ends the task and clears
 * fadeActive; below 1.0 it writes the progress to the target's alpha.
 */
void fadeInStep(FadeTask *task) {
    FadeOwner *owner;
    struct XenoActor *target;
    f32 progress;

    owner = task->payload.fade.owner;
    progress = task->payload.fade.progress + task->payload.fade.step;
    task->payload.fade.progress = progress;
    if (progress >= 1.0f) {
        target = owner->target;
        target->drawFlags &= ~ACTOR_STATE_FADING;
        ov01TaskRelease(task);
        owner->fadeActive = 0;
        return;
    }
    owner->target->alpha = progress;
}

/*
 * fadeOutStart: starts a fade-out of the owner's
 * target from progress 1.0 with step 0.1, with the callback fadeOutStep,
 * and sets the 0x10000 flag on the target.
 */
FadeTask *fadeOutStart(FadeOwner *owner) {
    FadeTask *task = ov01TaskClaim();

    if (task != 0) {
        task->payload.fade.progress = 1.0f;
        task->payload.fade.step = FADE_DEFAULT_STEP;
        task->payload.fade.owner = owner;
        task->update = fadeOutStep;
        task->draw = 0;
        owner->fadeActive = task;
        owner->target->drawFlags |= ACTOR_STATE_FADING;
    }
    return task;
}

/*
 * fadeOutStep: the per-frame step of the fade-out
 * task. At 0.0 it sets the target's alpha to 0, ends the task and
 * clears fadeActive; the 0x10000 flag stays set.
 */
void fadeOutStep(FadeTask *task) {
    FadeOwner *owner;
    f32 progress;

    owner = task->payload.fade.owner;
    progress = task->payload.fade.progress - task->payload.fade.step;
    task->payload.fade.progress = progress;
    if (progress <= 0.0f) {
        owner->target->alpha = 0.0f;
        ov01TaskRelease(task);
        owner->fadeActive = 0;
        return;
    }
    owner->target->alpha = progress;
}
