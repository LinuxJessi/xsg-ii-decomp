/*
 * OV01 original TU 21: 0x00a88cb0..0x00a88e70 (4 functions)
 */
#include "common.h"
#include "ov01/tu001.h"

/*
 * fadeInStart (proposed fadeInStart): starts a fade-in of the owner's target.
 * It takes a FadeTask at progress 0 with step 0.1, sets the 0x10000 flag on
 * the target, and stores the task in owner+0x94. Callers are not in the
 * dossier.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu021", fadeInStart);

#include "shared.h"

/*
 * Partial views of the fade task and of the records it reaches, evidenced by
 * the two claimed functions only.  Nothing else of these records is modeled.
 * ActorView is the same actor view tu018 and tu020 use.
 */
typedef struct ActorView {
    unsigned char unmodeled_00[0xBC];
    u32 stateFlags;     /* +0xBC: bit 0x00010000 is cleared when the fade ends */
    f32 transparency;   /* +0xC0: the alpha the fade drives */
} ActorView;

typedef struct FadeOwner {
    unsigned char unmodeled_00[0x90];
    ActorView *target;  /* +0x90 */
    int fadeActive;    /* +0x94: holds the running FadeTask (fadeInStart and
                        * fadeOutStart store it; the asm writes a pointer
                        * here, so the type should become FadeTask *).
                        * Cleared to 0 when the fade ends. */
} FadeOwner;

typedef struct FadeTask {
    unsigned char unmodeled_00[0xC];
    FadeOwner *owner;   /* +0xC */
    f32 progress;       /* +0x10: advanced by step each frame */
    f32 step;           /* +0x14 */
} FadeTask;


/*
 * fadeInStep (proposed fadeInStep): the per-frame step of the fade-in
 * task. At 1.0 it clears the target's 0x10000 flag, ends the task and clears
 * fadeActive; below 1.0 it writes the progress to the target's transparency.
 */
void fadeInStep(FadeTask *task) {
    FadeOwner *owner;
    ActorView *target;
    f32 progress;

    owner = task->owner;
    progress = task->progress + task->step;
    task->progress = progress;
    if (progress >= 1.0f) {
        target = owner->target;
        target->stateFlags &= 0xFFFEFFFF;
        ov01TaskRelease(task);
        owner->fadeActive = 0;
        return;
    }
    owner->target->transparency = progress;
}

/*
 * fadeOutStart (proposed fadeOutStart): starts a fade-out of the owner's
 * target from progress 1.0 with step 0.1, with the callback fadeOutStep,
 * and sets the 0x10000 flag on the target.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu021", fadeOutStart);

/*
 * fadeOutStep (proposed fadeOutStep): the per-frame step of the fade-out
 * task. At 0.0 it sets the target's transparency to 0, ends the task and
 * clears fadeActive; the 0x10000 flag stays set.
 */
void fadeOutStep(FadeTask *task) {
    FadeOwner *owner;
    f32 progress;

    owner = task->owner;
    progress = task->progress - task->step;
    task->progress = progress;
    if (progress <= 0.0f) {
        owner->target->transparency = 0.0f;
        ov01TaskRelease(task);
        owner->fadeActive = 0;
        return;
    }
    owner->target->transparency = progress;
}
