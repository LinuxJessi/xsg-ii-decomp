/*
 * OV01 original TU 50: 0x00aa3928..0x00aa3e28 (7 functions)
 */
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", objPathInit);

typedef struct PathObject {
    unsigned char unmodeled_00[0x10];
    unsigned char mover[0x90 - 0x10];    /* +0x10: the mover; passed by address to slideSetStep (step) and objStdMove (arrived) */
    int state;                               /* +0x90: 1..2 while the mover is active */
    unsigned char unmodeled_94[0x9C - 0x94];
    int counter;                             /* +0x9C: stepped by 4, saturating at 0x18 */
} PathObject;

int objStdMove(void *target);
void slideSetStep(void *target, int value);

/* Per-frame update (callback +4): while the mover is active it steps toward its target,
 * clears state when objStdMove reports arrival, and advances the counter to 0x18. */
void objMoveUpdate(PathObject *object) {
    int state;
    void *target;
    int counter;

    state = object->state;
    if (state != 0) {
        if (state >= 0) {
            if (state < 3) {
                target = object->mover;
                slideSetStep(target, 2);
                if (objStdMove(target) != 0) {
                    object->state = 0;
                }
            }
        }
    }
    counter = 0x18;
    if (object->counter < 0x18) {
        counter = object->counter + 4;
    }
    object->counter = counter;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", func_00AA3AA0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", objPathReset);

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", objPathStep);

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", func_00AA3CF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu050", func_00AA3D58);
