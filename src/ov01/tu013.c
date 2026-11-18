/*
 * OV01 original TU 13: 0x00a85a80..0x00a862d8 (9 functions)
 */
#include "common.h"
#include "ov01/tu012.h"
#include "ov01/tu016.h"
#include "ov01/tu033.h"
#include "ov01/tu071.h"

/* Eight pose frames of the ramp that command 0x1B runs (src/ov01/tu015.c calls it the eight-frame ramp). */
#define UNIT_POSE_RAMP_FRAMES 8

void unitValueRefresh(Ov01Object *object);
int calcStatGet(Ov01Object *object, int kind, int mask);
void calcStatResetCha(Ov01Object *object);
void calcStatSet(Ov01Object *object, int kind, int value);
void streamPlayByKey(Ov01Object *object, int key);
void unitRemove(Ov01Object *object);
int unitCmdCodeGet(int code, int wait);
int unitCmdModeGet(int mode);

/*
 * Command 0x1B: sets the mode from the step code, then ramps pose->snapshot.vector.y over
 * UNIT_POSE_RAMP_FRAMES frames (unitPoseRamp and objPosePublish each frame). The
 * last step clears flags bit 0 and calls unitEffectStart(object, 310, 21).
 */
int unitCmdModeRamp(Ov01Object *object, Ov01Command *step)
{
    Ov01Pose *pose = &object->pose;

    switch (step->state) {
    case 0:
        unitSetPhase(object, unitCmdCodeGet(step->code, step->arg.wait));
        step->poseIndex = 0;
        object->stats->state70 = unitCmdModeGet(step->code);
        step->state = step->state + 1;
        break;
    case 1:
        if (step->poseIndex < UNIT_POSE_RAMP_FRAMES) {
            pose->snapshot.vector.y = unitPoseRamp(step->poseIndex, 0);
            objPosePublish(object);
            step->poseIndex = step->poseIndex + 1;
        } else {
            step->state = 2;
        }
        break;
    default:
        object->flags &= ~1;
        unitEffectStart(object, 310, 21);
        return 1;
    }
    return 0;
}


/*
 * Command 0x27: starts the motion for the step code (unitSetPhase with
 * unitCmdCodeGet) and plays its voice. Code 3 also faces the target and copies
 * three pose floats. It waits for unitIsDone (code 3 leaves refresh to the
 * caller); other codes run unitRefreshPhase when done.
 */
int unitCmdModeStart(Ov01Object *object, Ov01Command *step)
{
    Ov01Pose *pose = &object->pose;

    if (step->state == 0) {
        object->stats->state70 = unitCmdModeGet(step->code);
        unitSetPhase(object, unitCmdCodeGet(step->code, step->arg.wait));
        if (step->code == 3) {
            seObjectVoiceStartDefault(object, 14);
        } else if (step->code == 1) {
            seObjectVoiceStartDefault(object, 16);
        } else {
            seObjectVoiceStartDefault(object, 15);
        }
        unitSetSelector(object, 6);
        object->flags &= ~1;
        if (step->code == 3) {
            float saved1;
            float saved0;
            float saved2;
            unitFaceTarget(object, step->arg.wait, step->faceTarget);
            saved1 = pose->snapshot.vector.y;
            saved0 = pose->point.x;
            saved2 = pose->point.z;
            pose->point.y = saved1;
            pose->snapshot.vector.x = saved0;
            pose->snapshot.vector.z = saved2;
            objPosePublish(object);
        }
        step->state = step->state + 1;
    } else if (step->code == 3) {
        if (unitIsDone(object) != 0) {
            return 1;
        }
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        return 1;
    }
    return 0;
}

/*
 * Command 0xA, the mode dispatcher: mode 4 with state 1 enters command 0x1B,
 * mode 4 with state 3 and mode 3 with state 2 finish at once, and anything else
 * runs command 0x27.
 */
int unitCmdModeDispatch(Ov01Object *object, Ov01Command *step)
{
    if (step->code == 4) {
        if (object->stats->state70 == 1) {
            step->command = UNIT_CMD_MODE_RAMP;
            seObjectVoiceStartDefault(object, 13);
            return unitCmdModeRamp(object, step);
        }
        if (object->stats->state70 == 3) {
            object->stats->state70 = unitCmdModeGet(4);
            object->flags &= ~1;
            seObjectVoiceStartDefault(object, 13);
            return 1;
        }
    }
    if (step->code == 3 && object->stats->state70 == 2) {
        object->stats->state70 = unitCmdModeGet(3);
        object->flags &= ~1;
        seObjectVoiceStartDefault(object, 14);
        return 1;
    }
    step->command = UNIT_CMD_MODE_START;
    return unitCmdModeStart(object, step);
}

/*
 * Maps a command mode to the code of its move. Modes 1 and 2 share one entry;
 * a mode outside 0..4 gives 1. wait selects the second code of each pair.
 */
int unitCmdCodeGet(int code, int wait)
{
    int result;

    switch (code) {
    case 0:
        if (wait == 0) {
            result = 28;
            break;
        }
        result = 29;
        break;
    case 1:
    case 2:
        if (wait == 0) {
            result = 30;
            break;
        }
        result = 31;
        break;
    case 3:
        if (wait == 0) {
            result = 36;
            break;
        }
        result = 39;
        break;
    case 4:
        if (wait == 0) {
            result = 32;
            break;
        }
        result = 33;
        break;
    default:
        result = 1;
        break;
    }
    return result;
}

/* Maps the command mode (2, 3, 4) to its mode index; any other value maps to 0. */
int unitCmdModeGet(int mode)
{
    int result;

    switch (mode) {
    case 3:
        result = 2;
        break;
    case 2:
        result = 1;
        break;
    case 4:
        result = 3;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

/* Command 0xB, one-shot start: issues command 0x18 and effect 0xB, then waits for the command to finish. */
int func_00A85EA0(Ov01Object *object, Ov01Command *step)
{
    if (step->state == 0) {
        unitSetPhase(object, UNIT_CMD_MOVE_CHA_JUMP);
        seObjectVoiceStartDefault(object, 0xB);
        step->state = step->state + 1;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        object->flags &= ~1;
        return 1;
    }
    return 0;
}

/* Same shape as func_00A85EA0 with command 0x19 and effect 0xC. */
int func_00A85F30(Ov01Object *object, Ov01Command *step)
{
    if (step->state == 0) {
        unitSetPhase(object, UNIT_CMD_MOVE_CHA_HOVER);
        seObjectVoiceStartDefault(object, 0xC);
        step->state = step->state + 1;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        object->flags &= ~1;
        return 1;
    }
    return 0;
}

/*
 * Command 0xD: on the first call resets the stat masks and plays stream 4.
 * Phase 1 waits for the position check and, at mode 16 or more, fades out
 * (unitFadeOut); phase 2 waits for the fade to end, and phase 3 removes
 * the unit with unitRemove.
 */
int func_00A85FC0(Ov01Object *object, Ov01Command *step)
{
    StatBlock *motion;
    int keyHigh;
    int keyLow;
    int mask;
    short mode;
    short key;

    switch (step->state) {
    case 0:
        object->flags |= 4;
        if (object->stats->state70 != 2) {
            unitSetPhase(object, 42);
        } else if (func_00A86FF0(object) != 0) {
            unitSetPhase(object, 42);
        }
        if (object->stats->state70 == 1) {
            object->stats->state70 = 0;
        }
        unitValueRefresh(object);
        mask = calcStatGet(object, 3, 0xffff);
        calcStatResetCha(object);
        if (mask & 0x400) {
            calcStatSet(object, 3, 1024);
        }
        if (mask & 0x800) {
            calcStatSet(object, 3, 2048);
        }
        if (mask & 0x2000) {
            calcStatSet(object, 3, 8192);
        }
        if (mask & 0x4000) {
            calcStatSet(object, 3, 16384);
        }
        object->stats->entryCount = 0;
        object->stats->comboHits = 0;
        seObjectVoiceStartDefault(object, 5);
        streamPlayByKey(object, 4);
        if ((object->stats->flags & 0x2000) != 0) {
            step->state = step->state + 1;
        } else {
            unitMoveKey(object, &keyHigh, &keyLow);
            unitEffectStart(object, keyHigh, keyLow);
            step->state = step->state + 1;
        }
        return 0;
    case 1:
        mode = object->stats->key;
        unitMoveKey(object, &keyHigh, &keyLow);
        if (mode != 125) {
            /* The key test takes the object as an int (ov01/tu012.h); its first argument is unused. */
            if (unitEffectKeyFree((int) object, keyHigh, keyLow) == 0) {
                break;
            }
        } else {
            if (object->commandQueue.flag < 241) {
                break;
            }
        }
        motion = object->stats;
        key = motion->key; /* read before the flag store: a union member (alias set 0) would not hoist */
        object->flags &= ~1;
        if (key >= 16) {
            if (motion->flags & 0x80) {
                step->state = step->state + 2;
            } else {
                unitFadeOut(object);
                step->state = step->state + 1;
            }
            break;
        }
        return 1;
    case 2:
        if (object->fade == 0) {
            step->state = 3;
        }
        break;
    case 3:
        if (object->stats->key >= 16) {
            unitRemove(object);
        }
        return 1;
    default:
        break;
    }
    return 0;
}

/*
 * Command 0xE: issues the step code; when step->arg.wait is set it also waits
 * for unitIsDone.
 */
int unitCmdCodeWait(Ov01Object *object, Ov01Command *step)
{
    int code;
    int wait;

    code = step->code;
    wait = step->arg.wait;
    if (step->state == 0) {
        unitSetPhase(object, code);
        step->state = step->state + 1;
    } else {
        if (wait == 0) {
            return 1;
        }
        if (unitIsDone(object) != 0) {
            return 1;
        }
    }
    return 0;
}
