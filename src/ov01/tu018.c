/*
 * OV01 original TU 18: 0x00a88348..0x00a88a50 (10 functions)
 */
#include "common.h"
#include "ov01/tu001.h"

/*
 * unitFadeIn (proposed unitFadeIn): starts a fade-in of a unit. It takes a
 * fade task from func_00A802F8 at progress 0 with step 0.1, sets the 0x10000
 * flag on the unit's motion actor (+0x1D0) and its slot actors, and records
 * the task at unit+0x1E8. Callers include unitBind.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeIn);

/*
 * unitFadeInStep (proposed unitFadeInStep): per-frame step of unitFadeIn's
 * task. It writes the progress as transparency to the motion actor and the
 * slot actors, and at 1.0 clears the 0x10000 flag and ends the task.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeInStep);

/*
 * unitFadeOut (proposed unitFadeOut): the fade-out counterpart of
 * unitFadeIn, starting at progress 1.0 and ending in func_00A95608.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeOut);

/*
 * unitFadeOutStep (proposed unitFadeOutStep): per-frame step of unitFadeOut's
 * task. At 0.0 it sets transparency 0 on the motion actor and slot actors and
 * ends the task.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeOutStep);

/*
 * unitSlotFadeIn (proposed unitSlotFadeIn): fades in the actor in slot `index`
 * (unit+0x1DC + 4*index) with the TransparencyFade callback unitSlotFadeInStep.
 * It clears bit 0x4 and sets 0x10000 on that actor.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitSlotFadeIn);

/*
 * Partial view of the Actor that a transparency fade drives. This TU touches
 * only the flags word at +0xBC and transparency at +0xC0; nothing here takes
 * sizeof(ActorView).
 */
typedef struct ActorView ActorView;

struct ActorView {
    unsigned char unmodeled_00[0xBC];
    unsigned int stateFlags; /* +0xBC: unitSlotFadeInStep clears bit 0x00010000 at
                              * full opacity; unitSlotFadeOutStep sets bit 0x4 when
                              * the fade runs out. */
    float transparency;      /* +0xC0: set to the running progress, or 0.0 */
};

/*
 * Fade record passed as the only argument of unitSlotFadeInStep and
 * unitSlotFadeOutStep. progress is the running value that is written to the
 * actor's transparency; step is added to it or subtracted from it per call.
 */
typedef struct TransparencyFade TransparencyFade;

struct TransparencyFade {
    unsigned char unmodeled_00[0xC];
    ActorView *actor;  /* +0xC */
    float progress;    /* +0x10 */
    float step;        /* +0x14 */
};

/* ov01TaskRelease ends a fade task (called by every fade step, unnamed here). */

/*
 * unitSlotFadeInStep (proposed unitSlotFadeInStep): the step of unitSlotFadeIn's
 * task. Progress rises by step; at 1.0 the actor's 0x10000 flag is cleared
 * and the task ends.
 */
void unitSlotFadeInStep(TransparencyFade *fade) {
    ActorView *actor;
    float progress;

    actor = fade->actor;
    progress = fade->progress + fade->step;
    fade->progress = progress;
    if (progress >= 1.0f) {
        actor->stateFlags &= 0xFFFEFFFF;
        ov01TaskRelease(fade);
        return;
    }
    actor->transparency = progress;
}

/*
 * unitSlotFadeOut (proposed unitSlotFadeOut): fades out the actor in slot
 * `index` from progress 1.0 with the callback unitSlotFadeOutStep. Sets 0x10000.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitSlotFadeOut);

/*
 * unitSlotFadeOutStep (proposed unitSlotFadeOutStep): the step of unitSlotFadeOut's
 * task. Progress falls by step; at 0.0 the actor becomes transparent, bit
 * 0x4 is set (the actor is hidden), and the task ends.
 */
void unitSlotFadeOutStep(TransparencyFade *fade) {
    ActorView *actor;
    float progress;

    actor = fade->actor;
    progress = fade->progress - fade->step;
    fade->progress = progress;
    if (progress <= 0.0f) {
        actor->transparency = 0.0f;
        actor->stateFlags |= 4;
        ov01TaskRelease(fade);
        return;
    }
    actor->transparency = progress;
}

/*
 * unitFadeTo (proposed unitFadeTo): core of the unitFade script command
 * (unitFade forwards unit, its second argument and its third argument). It
 * takes a fade task, sets 0x10000 on the unit's motion actor and slot actors,
 * and records the task at unit+0x1E8. The task target is the third argument
 * divided by 100; its step is (target - current transparency) divided by the
 * second argument, so the second argument reads as a frame count.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeTo);

/*
 * unitFadeToStep (proposed unitFadeToStep): the per-frame step of
 * unitFadeTo's task. It advances progress by step, writes it to the
 * motion actor and the slot actors that do not carry the 0x4000 part flag,
 * and ends the task once the remaining distance is below |step|.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu018", unitFadeToStep);
