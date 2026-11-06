/*
 * OV01 original TU 89: 0x00ab3668..0x00ab40d8 (6 functions)
 */
#include "common.h"
#include "main/tu197.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu089", MEfDrawPathTrail);

/*
 * Partial view of the pooled effect object that the callbacks of this TU
 * receive. Its three per-frame callbacks sit in the prefix (+0x08, +0x10 and
 * +0x14, stored by the creators in ov01/tu088 and ov01/tu089). The work record
 * starts at +0x20. Only the step count at +0x74 and the status word at +0xE0
 * of the object are touched in this TU.
 */
typedef struct EffectWork EffectWork;

struct EffectWork {
    unsigned char unmodeled_00[0x54];
    unsigned short stepCount;   /* +0x54 of the work (+0x74 of the object): frames since creation */
    unsigned char unmodeled_56[0xC0 - 0x56];
    unsigned short statusFlags; /* +0xC0 of the work (+0xE0 of the object) */
};

typedef struct EffectObject EffectObject;

struct EffectObject {
    unsigned char unmodeled_00[0x20]; /* +0x08, +0x10, +0x14: per-frame callbacks */
    EffectWork work; /* +0x20 */
};

short ov01LastValueGet(EffectObject *object);

/*
 * Post-process callback. Runs only while ov01LastValueGet returns 0; counts one
 * step and ends the effect once the step reaches 17 (0x11).
 */
void MEfAdvancePathStep(EffectObject *object) {
    EffectWork *work;
    int nextStepCount;

    work = &object->work;
    if (ov01LastValueGet(object) != 0) {
        return;
    }
    nextStepCount = work->stepCount + 1;
    work->stepCount = nextStepCount;
    if ((short) nextStepCount >= 0x11) {
        MSeObjRequestFree(object);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu089", func_00AB3CC8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu089", func_00AB3DC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu089", MEfBuildPathSamples);

/*
 * Status callback. While the effect runs, statusFlags follows the step count:
 * 1 on steps 0 to 19, 0x11 on step 0 and bit 0x20 added on step 19 (0x13).
 * Step 20 and later clear it.
 */
void MEfUpdatePathStatus(EffectObject *object) {
    EffectWork *work;
    short stepCount;

    work = &object->work;
    if (ov01LastValueGet(object) == 0) {
        stepCount = work->stepCount;
        work->statusFlags = 0;
        if (stepCount < 0x14) {
            work->statusFlags = 1;
            if (stepCount == 0) {
                work->statusFlags = 0x11;
            }
            if (stepCount == 0x13) {
                work->statusFlags |= 0x20;
            }
        }
    }
}
