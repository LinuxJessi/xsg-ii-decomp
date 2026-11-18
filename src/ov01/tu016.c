/*
 * OV01 original TU 16: 0x00a86cb0..0x00a87e38 (24 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/xenoActor.h"
#include "ov01/tu005.h"
#include "ov01/tu012.h"
#include "ov01/tu016.h"
#include "ov01/tu033.h"
#include "ov01/tu042.h"
#include "ov01/tu070.h"
#include "ov01/tu071.h"

/* Object flag bit 0x1: unitRunCommands sets it while a command entry runs; the handlers that
 * finish an entry clear it (tu015 unitCmdFinish does the same). */
#define UNIT_FLAG_ACTIVE 0x1

/* unitCmdTurnAim snaps the angle to the heading once the object has waited this many runs. */
#define TURN_SNAP_WAITS 5

/* The pose ramp table has eight entries (unitPoseRamp). */
#define POSE_RAMP_COUNT 8

/* Object flag bit 0x4: set by unitQueueMoveCommands once the object's moves are queued. */
#define UNIT_FLAG_QUEUED 0x4

/* The records of tu012.h (Ov01Object, Ov01Motion, Ov01Pose, the actor at +0x1D0 and the command record) are
 * the ones these functions touch. */

/* The limit block of the actor's pointer at +0x5EC; only its four extents are read. */
typedef struct Ov01Limit {
    unsigned char unmodeled_00[0x90];
    f32 extentA; /* +0x90 */
    unsigned char unmodeled_94[0x4];
    f32 extentB; /* +0x98 */
    unsigned char unmodeled_9C[0x4];
    f32 extentC; /* +0xA0 */
    unsigned char unmodeled_A4[0x4];
    f32 extentD; /* +0xA8 */
} Ov01Limit;

/*
 * The unit actor as tu016 reads it: the full record is struct XenoActor of main/tu051.h, which this file does not
 * include. objPosePublish and objPoseFetch move the position as two 8-aligned words, an alias-0 access; a u64 view
 * inside the shared XenoActor.position would give every access of main's code alias set 0 and change the code of
 * uwamonoPlayerUpdate and uwamonoParabolaMove, and a union around Ov01Object.unit does the same to tu012 and tu018.
 * Fields: snapshot = XenoActor.position, heading = rotation.y, limit = rsrc[XAct_Rsrc_MDL],
 * timing_flag = rsrc[XAct_Rsrc_5].
 */
struct XenoActor {
    unsigned char unmodeled_00[0x10];
    DoublewordVector4 snapshot; /* +0x10 */
    unsigned char unmodeled_20[0x34];
    f32 heading; /* +0x54 */
    unsigned char unmodeled_58[0x594];
    Ov01Limit *limit; /* +0x5EC */
    unsigned char unmodeled_5f0[0x10];
    u32 timing_flag; /* +0x600: nonzero when unitSetTimedPhase times the unit and its child (tu016: the work-pending flag) */
};

typedef struct Ov01Lookup Ov01Lookup;

typedef struct Ov01Sequence Ov01Sequence;

/* A table row addressed by a key, its word at +0x08. */
typedef union Ov01LookupWord {
    unsigned short value;
    int index;
    float scale;
} Ov01LookupWord;

struct Ov01Lookup {
    unsigned char unmodeled_00[0x8];
    Ov01LookupWord entryWord;   /* +0x08: read as a row value, an index or a float */
    unsigned char unmodeled_0C[0x10 - 0x0C];
    float goalZ;                /* +0x10: the z goal when the mode is neither 0 nor 1 */
};

struct Ov01Sequence {
    unsigned char unmodeled_00[0x4];
    int step;                   /* +0x04 */
    int loadId;                 /* +0x08: the snapshot id a load passes on */
};

/* Prototypes of this TU's functions and of the callees. */
int slideAngleStep(void *block);
float slideHorizontalDistance(void *block);
void slideLimitGoalXZ(void *block, f32 numerator);
int objStdMove(void *block);
float objPoseHorizontalDistance(Ov01Object *object);
float objPoseSlideLimit(Ov01Object *unusedFirst, Ov01Object *value, int rowKey);
float unitDefArea(Ov01Object *object);
int *effectPosGet(int index);
void unitRemove(void);
void XenoActorClearChannel(struct XenoActor *source);
void func_0028B158(void *source, int first, int second, int third);
void runEffGet(int *first, int *second);
void sefApplyRecordMode(struct XenoActor *source, int key, int mode);
void slideSnapPosToGoal(void *block);
float atan2f(float y, float x);

int func_00A86CB0(Ov01Object *object) {
    object->flags &= ~UNIT_FLAG_ACTIVE;
    return 1;
}

/* One step of the pose for the step record. Its first run aims the goal and
 * the heading; every run steps the angle, publishes the pose and returns
 * whether the angle arrived. After five waits the angle snaps to the heading
 * and the call reports done. */
int unitCmdTurnAim(Ov01Object *object, Ov01Command *step) {
    Ov01Pose *pose;
    int arrived;

    pose = &object->pose;
    if (step->state == 0) {
        unitCmdDstSet(object, 3, step->poseIndex == 0 ? 0 : 2);
        objPoseHeadingUpdate(object, 0);
        if (object->stats->state >= 4) {
            pose->headingDelta *= 0.25f;
        }
        step->state += 1;
    }
    arrived = slideAngleStep(pose);
    objPosePublish(object);
    if (object->commandQueue.flag >= TURN_SNAP_WAITS) {
        pose->angle = pose->heading;
        return 1;
    }
    return arrived != 0;
}

int unitCmdStreamPlay(int value) {
    seVoiceStartTagged(2, 2);
    streamPlayByKey(value, 3);
    return 1;
}

int unitCmdRemove(void) {
    unitRemove();
    return 1;
}

int unitCmdFinishRefresh(Ov01Object *object) {
    /* mode 2 and mode 3 skip the call */
    if ((unsigned int) ((unsigned char) object->stats->state70 - 2) >= 2U) {
        unitRefreshPhase(object);
    }
    object->flags &= ~UNIT_FLAG_ACTIVE;
    return 1;
}

/* Clears the object's pending work: slot 2 is finished when the key's row byte
 * is 5, the source's busy flag is cleared, and each set slot finishes its work
 * when busy. */
void unitSlotsClear(Ov01Object *object, int rowKey) {
    void *fifo;
    void *list;
    struct XenoActor *source;
    TecRecord *row;
    int slot;

    cmdListPtrGet(&fifo, &list);
    if (ov01FifoGetUsed(fifo) == 0) {
        object->flags |= 0x800;
    }
    row = tecRecordGet(rowKey);
    if (row->gateFlag == 5) {
        source = object->children[2];
        if (source != 0) {
            source->timing_flag = 0;
            XenoActorRelease(source);
            object->children[2] = 0;
        }
    }
    source = object->unit;
    source->timing_flag = 0;
    for (slot = 0; slot < 3; slot++) {
        source = object->children[slot];
        if (source != 0) {
            if (source->timing_flag != 0) {
                XenoActorClearChannel(source);
            }
            source->timing_flag = 0;
        }
    }
}

/* Clears the object's flag bits 0x100 and 0x40000 (no name is known for them yet). */
void func_00A86F20(Ov01Object *object) {
    object->flags &= ~(0x100 | 0x40000);
}

/* Queues the object's commands for its motion mode: mode 3 queues 29 first,
 * and mode 2 queues 28 as well when func_00A86FF0 says so; command 13 always
 * follows. The object gets flag 0x4 and the call returns 1, or it returns 0
 * when it is already flagged or its motion is held. */
int unitQueueMoveCommands(Ov01Object *object) {
    Ov01Command command;

    if ((object->flags & UNIT_FLAG_QUEUED) != 0) {
        return 0;
    }
    if (object->stats->hp == 0) {
        if (object->stats->state70 == 3) {
            command.command = 29;
            unitQueueAppend(object, &command);
        }
        if (object->stats->state70 == 2) {
            if (func_00A86FF0(object) != 0) {
                command.command = 28;
                unitQueueAppend(object, &command);
            }
        }
        command.command = 13;
        unitQueueAppend(object, &command);
        object->flags |= UNIT_FLAG_QUEUED;
        return 1;
    }
    return 0;
}

int func_00A86FF0(Ov01Object *object) {
    switch (object->stats->key) {
    case 110:
    case 115:
    case 119:
    case 122:
    case 124:
    case 126:
    case 139:
    case 140:
        return 1;
    }
    return 0;
}

/* Sets the pose goal (x, y, z, w) from one of three sources: mode 0 reads the
 * partner's motion, mode 1 reads the lookup entry's index, and any other mode
 * reads the entry's floats. With cmdId set and mode 0 the z goal is limited
 * against the partner's slide limit. */
void unitCmdDstSet(Ov01Object *object, int cmdId, int mode) {
    Ov01Lookup *lookup;
    Ov01Object *partner;
    float *goal;
    int partnerKey;
    int posIndex;
    int *posRow;
    float diff;

    goal = &object->pose.point.x;
    lookup = dataLookupCurrent(&object->commandQueue);
    if (mode == 0) {
        partner = object->partner;
        partnerKey = object->partnerKey;
        posIndex = partner->stats->posIndex;
        posRow = effectPosGet(posIndex);
        goal[0] = (float) posRow[0] / 100;
        /* the original stores the word zero (sw $0) into goal[1] here */
        ((int *) goal)[1] = 0;
        posRow = effectPosGet(posIndex);
        goal[2] = (float) posRow[2] / 100.0f;
        if (cmdId != 0) {
            diff = objPoseHorizontalDistance(object) - objPoseSlideLimit(object, partner, partnerKey);
            if (0.0f > diff) {
                diff = 0.0f;
            }
            slideLimitGoalXZ(&object->pose, diff);
        }
    } else if (mode == 1) {
        posIndex = lookup->entryWord.index;
        posRow = effectPosGet(posIndex);
        goal[0] = (float) posRow[0] / 100.0f;
        goal[1] = 0.0f;
        posRow = effectPosGet(posIndex);
        goal[2] = (float) posRow[2] / 100.0f;
    } else {
        goal[0] = lookup->entryWord.scale;
        goal[1] = 0.0f;
        goal[2] = lookup->goalZ;
    }
    goal[3] = 1.0f;
}

void objPoseSetSteps(Ov01Object *object, int mode, int unusedFlag) {
    Ov01Pose *pose;

    pose = &object->pose;
    objPoseHorizontalDistance(object);
    pose->easeX = 0.0f;
    pose->easeZ = 0.0f;
    pose->stepX = (pose->point.x - pose->snapshot.vector.x) / 15.0f;
    pose->stepZ = (pose->point.z - pose->snapshot.vector.z) / 15.0f;
    if (mode == 3 || mode == 4) {
        pose->easeY = -0.0204375f;
        pose->stepY = 0.15328124f;
    } else {
        pose->easeY = 0.0f;
        pose->stepY = (pose->point.y - pose->snapshot.vector.y) / 15.0f;
    }
}

float objPoseHorizontalDistance(Ov01Object *object) {
    return slideHorizontalDistance(&object->pose);
}

/* Blends the pose snapshot toward its start by numerator (tail call; the
 * second argument passes through unchanged to slideLimitGoalXZ). */
void objPoseBlend(Ov01Object *object, f32 numerator) {
    slideLimitGoalXZ(&object->pose, numerator);
}

/* One step of the pose snapshot, then the publish; returns the step's result. */
int objPoseStep(Ov01Object *object) {
    int arrived;

    arrived = objStdMove(&object->pose);
    objPosePublish(object);
    return arrived;
}

/* unusedFirst keeps its argument slot; the body never reads it. */
float objPoseSlideLimit(Ov01Object *unusedFirst, Ov01Object *value, int rowKey) {
    float base;

    if (value != 0) {
        base = unitDefArea(value);
    } else {
        base = 0.0f;
    }
    return base + ((float) tecRecordGet(rowKey)->scale / 100.0f);
}

/* Half the summed absolute extents of the limit's two axis pairs, the larger
 * of the two; when the motion sets a percent it returns that percent as a
 * fraction instead. */
float unitDefArea(Ov01Object *object) {
    Ov01Limit *limit;
    float result;
    float pairAC;
    float pairBD;
    float magnitude;
    float other;

    if (object->stats->slidePercent == 0) {
        limit = object->unit->limit;
        magnitude = limit->extentA;
        if (magnitude < 0.0f) {
            magnitude = -magnitude;
        }
        other = limit->extentC;
        if (other < 0.0f) {
            other = -other;
            pairAC = (magnitude + other) * 0.5f;
        } else {
            pairAC = (magnitude + other) * 0.5f;
        }
        do {
            magnitude = limit->extentB;
        } while (0);
        if (magnitude < 0.0f) {
            magnitude = -magnitude;
        }
        other = limit->extentD;
        if (other < 0.0f) {
            other = -other;
            pairBD = (magnitude + other) * 0.5f;
        } else {
            pairBD = (magnitude + other) * 0.5f;
        }
        if (pairBD < pairAC) {
            return pairAC;
        }
        result = pairBD;
    } else {
        result = (float) object->stats->slidePercent / 100.0f;
    }
    return result;
}

/* Publishes the pose snapshot and angle to the source and to the motion. */
void objPosePublish(Ov01Object *object) {
    struct XenoActor *source;
    long long snapshotLow;
    float angle;

    do {
        snapshotLow = object->pose.snapshot.words[0];
    } while (0);
    source = object->unit;
    source->snapshot.words[0] = snapshotLow;
    do {
        source->snapshot.words[1] = object->pose.snapshot.words[1];
    } while (0);
    angle = object->pose.angle;
    source->heading = angle;
    object->stats->facing = angle;
}

/* Loads the pose snapshot from the source and returns its low word. */
long long objPoseFetch(Ov01Object *object) {
    struct XenoActor *source;
    long long snapshotLow;

    source = object->unit;
    func_0028B158(source, 1, 0, 1);
    snapshotLow = source->snapshot.words[0];
    object->pose.snapshot.words[0] = snapshotLow;
    object->pose.snapshot.words[1] = source->snapshot.words[1];
    return snapshotLow;
}

int unitCmdRun(Ov01Object *object, int cmdId, int flip) {
    Ov01Sequence *sequence;
    Ov01Pose *pose;
    struct {
        int group;
        int record;
    } effect;

    sequence = dataLookupCurrent(&object->commandQueue);
    pose = &object->pose;
    runEffGet(&effect.group, &effect.record);
    switch (sequence->step) {
    case 0:
        unitSetPhase(object, 4);
        objPoseHeadingUpdate(object, 0);
        pose->headingDelta = pose->headingDelta / 10.0f;
        slideAngleStep(pose);
        objPosePublish(object);
        sequence->step += 1;
        /* fall through */
    case 1:
        if (unitIsDone(object) != 0) {
            unitSetPhase(object, 5);
            objPoseSetSteps(object, cmdId, flip);
            if (effect.group == 0 && effect.record == 0) {
                effect.record = 9;
                effect.group = object->stats->sequenceIndex;
            }
            unitEffectStart(object, effect.group, effect.record);
            seObjectVoiceStartDefault(object, 1);
            sequence->step++;
        } else {
            slideAngleStep(pose);
            objPosePublish(object);
        }
        break;
    case 2:
        if (objPoseStep(object) == 0) {
            break;
        }
        if (effect.group == 0 && effect.record == 0) {
            effect.record = 9;
            effect.group = object->stats->sequenceIndex;
        }
        sefApplyRecordMode(object->unit, (effect.group << 12) | effect.record, 6);
        return 1;
    }
    return 0;
}

/* One step of the object's sequence: the step in the looked-up entry selects
 * the branch, and the branch advances the phase. unusedFlag is the move flag tu014 passes; this body never reads it. */
int unitCmdMoveJump(Ov01Object *object, int phase, int unusedFlag) {
    Ov01Pose *pose;
    Ov01Sequence *sequence;
    float zScale;
    int posTarget;

    pose = &object->pose;
    sequence = dataLookupCurrent(&object->commandQueue);
    switch (sequence->step) {
    case 0:
        if (phase == 3) {
            phase = 4;
            posTarget = 0;
        } else {
            phase = 9;
            posTarget = 1;
        }
        objPoseHeadingUpdate(object, posTarget);
        slideAngleStep(pose);
        objPosePublish(object);
        zScale = objPoseHorizontalDistance(object);
        if (object->stats->state >= 4) {
            zScale /= 10.0f;
        } else {
            zScale /= 3.0f;
        }
        unitSetVectorZ(object, zScale);
        pose->headingDelta = zScale;
        unitSetPhase(object, phase);
        sequence->step++;
        break;
    case 1:
        if (!unitIsDone(object)) {
            break;
        }
        posTarget = (phase != 3);
        phase = 7;
        if (posTarget) {
            phase = 10;
        }
        seObjectVoiceStartDefault(object, 2);
        unitSetPhase(object, phase);
        sequence->step++;
        break;
    case 2:
        if (!unitIsDone(object)) {
            break;
        }
        if (phase == 3) {
            object->flags |= 0x20000;
            objPoseFetch(object);
            slideSnapPosToGoal(pose);
            objPosePublish(object);
            unitSetVectorZ(object, 1.0f);
            unitSetPhase(object, 6);
            return 1;
        }
        unitSetPhase(object, (phase == 4) ? 11 : 6);
        sequence->step++;
        break;
    case 3:
        if (!unitIsDone(object)) {
            break;
        }
        object->flags |= 0x20000;
        if (phase == 4) {
            objPoseFetch(object);
            slideSnapPosToGoal(pose);
            objPosePublish(object);
            unitSetVectorZ(object, 1.0f);
        }
        return 1;
    default:
        break;
    }
    return 0;
}

int unitCmdHoverFade(Ov01Object *object, int cmdId, int flip) {
    Ov01Pose *pose;
    Ov01Sequence *sequence;

    pose = &object->pose;
    sequence = dataLookupCurrent(&object->commandQueue);
    switch (sequence->step) {
    case 0:
        seObjectVoiceStartDefault(object, 1);
        unitFadeOut(object);
        sequence->step++;
        break;
    case 1:
        if (object->fade != 0) {
            break;
        }
        objPoseHeadingUpdate(object, cmdId != 5);
        slideAngleStep(pose);
        objPosePublish(object);
        if (flip == 0) {
            pose->snapshot.vector.x = pose->point.x;
            pose->snapshot.vector.y = pose->point.y;
            pose->snapshot.vector.z = pose->point.z;
            objPosePublish(object);
        } else {
            unitLoadSnapshot(object, sequence->loadId);
        }
        unitRefreshPhase(object);
        sequence->step++;
        break;
    case 2:
        if (cmdId == 5) {
            unitUnbind(object);
            return 1;
        }
        unitFadeIn(object);
        sequence->step++;
        break;
    case 3:
        if (object->fade == 0) {
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* Looks the object's key up; with action 1 it copies the entry's value into
 * the motion and runs unitRefreshPhase. */
void objMotionCopyLookup(Ov01Object *object, int action) {
    Ov01Lookup *lookup;

    lookup = dataLookupCurrent(&object->commandQueue);
    if (action == 1) {
        object->stats->posIndex = lookup->entryWord.value;
        unitRefreshPhase(object);
    }
}

/* Heading from the snapshot to the pose's current point (atan2 of the x and
 * z deltas), wrapped, with motion->facing kept when the two points coincide. */
int objPoseHeadingUpdate(Ov01Object *object, int faceAway) {
    Ov01Pose *pose;
    float delta;
    float stillMagnitude;
    float wrapMagnitude;
    float finalMagnitude;

    pose = &object->pose;
    if (pose->point.x == pose->snapshot.vector.x
        && pose->point.z == pose->snapshot.vector.z) {
        pose->heading = object->stats->facing;
    } else {
        pose->heading = atan2f(pose->point.x - pose->snapshot.vector.x,
                               pose->point.z - pose->snapshot.vector.z);
        if (faceAway == 1) {
            pose->heading += 3.1415927f;
        }
    }
    if (pose->heading < 0.0f) {
        pose->heading += 6.2831855f;
    }
    if (pose->heading >= 6.2831855f) {
        pose->heading -= 6.2831855f;
    }
    delta = pose->heading - pose->angle;
    if ((delta < 0.0f ? (stillMagnitude = -delta) : delta) <= 0.000001f) {
        pose->headingDelta = 0.0f;
        return 0;
    }
    if ((delta < 0.0f ? (wrapMagnitude = -delta) : delta) > 3.1415927f) {
        if (delta < 0.0f) {
            delta += 6.2831855f;
        } else {
            delta -= 6.2831855f;
        }
    }
    if ((delta < 0.0f ? (finalMagnitude = -delta) : delta) <= 0.000001f) {
        delta = 0.0f;
    }
    pose->headingDelta = delta;
    return 1;
}

/* The eight ramp values; reverse reads the table from its far end. */
float unitPoseRamp(int index, int reverse) {
    float rampTable[POSE_RAMP_COUNT] = {0.0f, 1.064f, 1.842f, 2.378f, 2.717f, 2.904f, 2.983f, 3.0f};
    float ramp = 0.0f;

    if (index < POSE_RAMP_COUNT) {
        int slot = index;
        if (reverse != 0) {
            slot = POSE_RAMP_COUNT - 1;
            slot -= index;
        }
        ramp = rampTable[slot];
    }
    return ramp;
}
