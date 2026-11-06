/*
 * OV01 original TU 65: 0x00aa8678..0x00aa8930 (3 functions)
 */
#include "common.h"

/*
 * Screen fade task (the object screenFadeInStart and screenFadeOutStart allocate).
 * The words used by the step and draw functions: alpha at 0x0C (GS alpha, 0x80
 * is opaque), alphaStep at 0x10, frameDelay at 0x14 and finished at 0x18.
 * Nothing else is evidenced.
 */
struct ScreenFade {
    unsigned char unmodeled_00[0x0C];
    int alpha;
    int alphaStep;
    int frameDelay;
    int finished;
};

/* Rising fade step: waits out frameDelay, then adds alphaStep; finished once alpha passes 0x80. */
void screenFadeInStep(struct ScreenFade *fade)
{
    int nextAlpha;

    if (fade->frameDelay > 0) {
        fade->frameDelay = fade->frameDelay - 1;
        return;
    }
    nextAlpha = fade->alpha + fade->alphaStep;
    fade->alpha = nextAlpha;
    if (nextAlpha >= 129) {
        fade->finished = 1;
    }
}

/* Falling fade step: waits out frameDelay, then subtracts alphaStep; finished once alpha reaches 0. */
void screenFadeOutStep(struct ScreenFade *fade)
{
    int nextAlpha;

    if (fade->frameDelay > 0) {
        fade->frameDelay = fade->frameDelay - 1;
        return;
    }
    nextAlpha = fade->alpha - fade->alphaStep;
    fade->alpha = nextAlpha;
    if (nextAlpha <= 0) {
        fade->finished = 1;
    }
}

/* Draws the fade as a full-screen sprite blended at the clamped alpha; a finished fade is removed and clears the running-fade global. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu065", screenFadeDraw);
