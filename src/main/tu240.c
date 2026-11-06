#include "common.h"
#include "main/tu242.h"
#include "main/tu244.h"

INCLUDE_ASM("asm/main/nonmatchings/tu240", timecodeInit);

#include "shared.h"

typedef struct TimecodeObject TimecodeObject;

typedef struct TimecodeStepper TimecodeStepper;

/*
 * The stepper at +0x28 of the timecode object. timecodeStepLevel and timecodeStepCounter take a
 * pointer to it (main 0x00285390 addiu $16,$4,40; 0x00285498 addiu $4,$4,40).
 * Offsets below are relative to the stepper. tu241 reads the first word as the
 * frame count (TimecodeRecord.elapsedFrames).
 */
struct TimecodeStepper {
    u64 elapsedFrames;            /* +0x00: frames at 30 per second; timecodeAdvance adds increment */
    int mode;                     /* +0x08: 0, 1 or 2; the range and phase code read it; timecodeSetMode sets it */
    int value_b;                  /* +0x0C: stored by timecodeSetValueB */
    unsigned char unmodeled_10[0x18];
    u64 increment;                /* +0x28: added to elapsedFrames by timecodeAdvance */
    unsigned char unmodeled_30[0x14];
    int level;                    /* +0x44: angle in degrees, 0 to 180 */
    int level_step;               /* +0x48: added to level by timecodeStepLevel */
    unsigned char unmodeled_4C[4];
    int computed_value;           /* +0x50: the cosine value from timecodeSampleCosine */
    unsigned char unmodeled_54[8];
    int counter;                  /* +0x5C: wraps to 0 at 20 in timecodeStepCounter */
};

/*
 * The timecode object every function of this TU receives in $a0. Each callee also
 * receives the same pointer unchanged, so the calls below pass it on. tu241 reads
 * the same object as TimecodeRecord. Only the members the functions of this TU
 * touch are named.
 */
struct TimecodeObject {
    u32 flags;
    int first_result;
    int second_result;
    unsigned char unmodeled_0C[0x10];
    int state;                    /* 0x1C: the functions act only while this is 3 */
    unsigned char unmodeled_20[8];
    TimecodeStepper stepper;
};


/* Runs msgSlotSetGeometry with the four values while the object is active (state 3). */
void timecodeTickIfActive(TimecodeObject *timecode, int first_value, int second_value, int third_value, int fourth_value)
{
    if (timecode->state == 3) {
        msgSlotSetGeometry(timecode, first_value, second_value, third_value, fourth_value);
    }
}

/* While active: flags = (flags & ~0x2) | 0x4. */
void timecodeSetFlag04(TimecodeObject *timecode)
{
    if (timecode->state == 3) {
        timecode->flags = (timecode->flags & ~2u) | 4u;
    }
}

/* Per-frame update, main 0x00285160. Returns at once unless the object is active and D_002B5D98 bit 0 is clear. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", timecodeUpdate);

/* Unconditional: elapsedFrames += increment. */
void timecodeAdvance(TimecodeObject *timecode)
{
    timecode->stepper.elapsedFrames += timecode->stepper.increment;
}

void func_002853F0(TimecodeObject *timecode);

/* Forwards to func_002853F0, the phase selector. */
void timecodeSelectPhase(TimecodeObject *timecode)
{
    func_002853F0(timecode);
}

/* Ramp step on +0x5C with +0x60 as the step, clamped at 180; returns 1 at the clamp or when flag 0x200 is clear. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_00285340);

int timecodeSampleCosine(TimecodeObject *timecode);

/*
 * Steps the level (angle) by level_step, clamps it at 180 and returns 1 when it
 * went past 180. Also stores the timecodeSampleCosine value in computed_value.
 */
int timecodeStepLevel(TimecodeObject *timecode)
{
    TimecodeStepper *stepper = &timecode->stepper;
    int clamped = 0;

    stepper->level += stepper->level_step;
    if (stepper->level >= 181) {
        clamped = 1;
        stepper->level = 180;
    }
    stepper->computed_value = timecodeSampleCosine(timecode);
    return clamped;
}

/* Phase selector on mode (+0x30) and index (+0x3C); writes a phase code 10 to 13 into +0x38 and +0x74. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_002853F0);

/* Wraps the counter (stepper +0x5C) to 0 at 20. */
void timecodeStepCounter(TimecodeObject *timecode)
{
    TimecodeStepper *stepper = &timecode->stepper;

    stepper->counter += 1;
    if (stepper->counter >= 20) {
        stepper->counter = 0;
    }
}

/* Cosine value: the centre of the range from func_00285548 with the half-width times cos(level). */
INCLUDE_ASM("asm/main/nonmatchings/tu240", timecodeSampleCosine);

/* Range pair for the mode and index, read from the table D_0063D430. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_00285548);

/* Flag-driven dispatch while active: flags 0x4, 0x2 and 0x8 each run func_0027EA80 (mode 0) or func_0027EB40. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_00285658);

/* While active: clears flag bits 0x4, 0x8, 0x40, 0x80 and 0x800, then sets 0x2. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_00285738);

/* While active: stores value in stepper +0x0C. */
void timecodeSetValueB(TimecodeObject *timecode, int value)
{
    if (timecode->state == 3) {
        timecode->stepper.value_b = value;
    }
}

void func_00285850(TimecodeObject *timecode);

/*
 * While active: sets the mode, then recomputes the duration (func_00285850), the
 * phase (func_002853F0) and the computed value (timecodeSampleCosine).
 */
void timecodeSetMode(TimecodeObject *timecode, int mode)
{
    if (timecode->state == 3) {
        timecode->stepper.mode = mode;
        func_00285850(timecode);
        func_002853F0(timecode);
        timecode->stepper.computed_value = timecodeSampleCosine(timecode);
    }
}

/* While active and index in 1 to 9: stores the index at +0x3C, recomputes as timecodeSetMode does and stores +0x78. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", timecodeSetIndex);

/* Duration from mode and index, stored at +0x10. */
INCLUDE_ASM("asm/main/nonmatchings/tu240", func_00285850);


/* Stores the results of sizedRecordCenterX and sizedRecordCenterY. */
void timecodeComputeResults(TimecodeObject *timecode)
{
    timecode->first_result = sizedRecordCenterX(timecode);
    timecode->second_result = sizedRecordCenterY(timecode);
}

/* While active: flags |= 0x40. The opposite of timecodeClearFlag40 in tu241. */
void timecodeSetFlag40(TimecodeObject *timecode)
{
    if (timecode->state == 3) {
        timecode->flags |= 0x40;
    }
}
