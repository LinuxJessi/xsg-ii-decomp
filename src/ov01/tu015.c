/*
 * OV01 original TU 15: 0x00a865d8..0x00a86cb0 (10 functions)
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu041.h"

/*
 * Unit parameter record reached through the unit's pointer at +0x144. index
 * (+0x6A) selects a row of the percentage table effectPosGet returns; mode
 * (+0x70) is the move-mode byte the step functions set and compare.
 */
struct UnitParam {
    unsigned char unmodeled_00[0x6A];
    s16 index;
    unsigned char unmodeled_6C[0x04];
    signed char mode;
};

/*
 * Three-float record at the unit's +0x150. primary and secondary are the two
 * table percentages divided by 100 (unitCmdMoveRatio sets them). ramp (+0x4) is
 * zeroed when a step starts there and is written each frame by the eight-frame
 * ramp in func_00A85A80 (src/ov01/tu013.c).
 */
struct UnitMoveRatio {
    f32 primary;
    int ramp;
    f32 secondary;
};

/* The unit's work record, the first argument of every function in this TU. */
struct UnitActor {
    unsigned char unmodeled_00[0x0C];
    int flags;
    unsigned char unmodeled_10[0x134];
    struct UnitParam *param;
    unsigned char unmodeled_148[0x08];
    struct UnitMoveRatio ratio;
};

/* Step record passed as the second argument: status is the command code, phase the step index. */
struct UnitStep {
    int status;
    int phase;
};

void func_00A849C8(struct UnitActor *object, int code);
int func_00A849B8(void *object);
void unitRefreshPhase(void *object);
int unitIsDone(void *object);
int objPosePublish(struct UnitActor *object);

/* Command 0xE, then waits for unitIsDone to report done. */
int func_00A865D8(struct UnitActor *object, struct UnitStep *step)
{
    switch (step->phase) {
    case 0:
        func_00A849C8(object, 0xE);
        step->phase++;
        /* fall through */
    case 1:
        if (unitIsDone(object) != 0) {
            step->phase++;
        }
        return 0;
    default:
        /* No return here: the original returns the constant 1 that the case-1 compare left in v0. */
        break;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu015", func_00A86658);

INCLUDE_ASM("asm/nonmatchings/ov01/tu015", func_00A866E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu015", func_00A86790);

/*
 * Percentage-driven move start: reads the row at param->index of the table,
 * stores primary and secondary as fractions, picks command 0x29 or 0x26 from
 * func_00A849B8, and waits for the move to finish.
 */
int unitCmdMoveRatio(struct UnitActor *object, struct UnitStep *step)
{
    struct UnitMoveRatio *ratio;
    s16 index;
    f32 primary;

    ratio = &object->ratio;
    if (step->phase == 0) {
        object->param->mode = 0;
        index = object->param->index;
        primary = (f32) *effectPosGet(index);
        ratio->ramp = 0;
        ratio->primary = primary / 100.0f;
        ratio->secondary = (f32) effectPosGet(index)[2] / 100.0f;
        objPosePublish(object);
        func_00A849C8(object, (func_00A849B8(object) ^ 0x27) == 0 ? 0x29 : 0x26);
        object->flags &= ~1;
        step->phase += 1;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu015", func_00A86968);

/* Finishes at once: unitRefreshPhase, clears the active flag, reports done. */
int unitCmdFinish(struct UnitActor *object)
{
    unitRefreshPhase(object);
    object->flags &= ~1;
    return 1;
}

/* Command 0x2C, then waits for unitIsDone; clears the active flag when done. */
int func_00A86B68(struct UnitActor *object, struct UnitStep *step)
{
    if (step->phase == 0) {
        func_00A849C8(object, 0x2C);
        step->phase++;
    } else if (unitIsDone(object) != 0) {
        object->flags &= ~1;
        return 1;
    }
    return 0;
}

/* Clears the active flag and reports done at once. */
int func_00A86BE8(struct UnitActor *object)
{
    object->flags &= ~1;
    return 1;
}

/*
 * Command 0x2B, then waits. When param->mode is 2 it sets step->status to
 * 0x1C instead and returns without issuing the command.
 */
int func_00A86C00(struct UnitActor *object, struct UnitStep *step)
{
    if (step->phase == 0) {
        object->flags &= ~4;
        if (object->param->mode == 2) {
            step->status = 0x1C;
            return 0;
        }
        func_00A849C8(object, 0x2B);
        step->phase++;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        object->flags &= ~1;
        return 1;
    }
    return 0;
}
