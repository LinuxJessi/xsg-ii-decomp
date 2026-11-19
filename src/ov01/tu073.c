/*
 * OV01 original TU 73: 0x00aaaa50..0x00aaab80 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/pad.h"
#include "ov01/tu073.h"

extern u16 textOptionFlags[];
extern u16 D_0063EDAC[];

/* Nonzero while bit 0x40 (PAD_CROSS) of textOptionFlags[0] is set: the back button of the debug menus. */
int configCancelPressed(void) {
    return textOptionFlags[0] & PAD_CROSS;
}

/* Up/down step of a debug menu value held with D-pad up or down; wraps at min and max. Returns nonzero when it changed. */
int debugValUD(int *value, int min, int max, int step) {
    int original = *value;
    int current = original;
    int increased = original + step;
    u16 heldButtons = D_0063EDAC[0];

    if (heldButtons & PAD_DOWN) {
        *value = increased;
        current = increased;
    } else if (heldButtons & PAD_UP) {
        int decreased = original - step;

        *value = decreased;
        current = decreased;
    }
    if (max < current) {
        *value = min;
        current = min;
    }
    if (current < min) {
        *value = max;
        current = max;
    }
    return current != original;
}

/* Left/right step of a debug menu value (R1/L1 move by ten steps); wraps at min and max. Returns nonzero when it changed. */
int debugValLR(int *value, int min, int max, int step) {
    int original = *value;
    int current = original;
    u16 heldButtons = D_0063EDAC[0];

    if (heldButtons & PAD_R1) {
        int raised = original + 10 * step;

        *value = raised;
        current = raised;
    }
    if (heldButtons & PAD_L1) {
        int lowered = current - 10 * step;

        *value = lowered;
        current = lowered;
    }
    if (heldButtons & PAD_RIGHT) {
        int shifted = current + step;

        *value = shifted;
        current = shifted;
    } else if (heldButtons & PAD_LEFT) {
        int shifted = current - step;

        *value = shifted;
        current = shifted;
    }
    if (max < current) {
        *value = min;
        current = min;
    }
    if (current < min) {
        *value = max;
        current = max;
    }
    return current != original;
}
