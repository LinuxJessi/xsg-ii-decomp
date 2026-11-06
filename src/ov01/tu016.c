/*
 * OV01 original TU 16: 0x00a86cb0..0x00a87e38 (24 functions)
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu070.h"

/*
 * Partial views of the records these functions touch. Only the words the
 * claimed functions read or write are modeled; the rest stays unmodeled.
 * Ov01Object and Ov01Motion are the same records tu012 models: the object's
 * flags at +0x0C, its motion pointer at +0x144, its pose at +0x150, and the
 * motion's angle at +0x6C (tu012 unitSetAngleRadians writes it).
 */
typedef struct Ov01Object Ov01Object;

typedef struct Ov01Motion Ov01Motion;

typedef struct Ov01Source Ov01Source;

typedef struct Ov01Lookup Ov01Lookup;

struct Ov01Object {
    unsigned char unmodeled_00[0xC];
    int flags;                  /* +0x0C */
    unsigned char lookupKey[0x144 - 0x10]; /* +0x10: its address is the key
                                            * objMotionCopyLookup passes to dataLookupCurrent */
    Ov01Motion *motion;         /* +0x144 */
    unsigned char unmodeled_148[0x150 - 0x148];
    long long poseSnapshotLow;  /* +0x150: pose snapshot, the start of the
                                 * blend and the step below */
    long long poseSnapshotHigh; /* +0x158 */
    unsigned char unmodeled_160[0x1D0 - 0x160];
    Ov01Source *source;         /* +0x1D0 */
};

struct Ov01Motion {
    unsigned char unmodeled_00[0x6A];
    unsigned short copiedValue; /* +0x6A: filled from a lookup entry */
    unsigned char unmodeled_6C[0x70 - 0x6C];
    unsigned char mode;         /* +0x70 */
};

/* The source record a pose is loaded from and published to. */
struct Ov01Source {
    unsigned char unmodeled_00[0x10];
    long long snapshotLow;      /* +0x10 */
    long long snapshotHigh;     /* +0x18 */
};

/* A table row addressed by a key, its value at +0x08. */
struct Ov01Lookup {
    unsigned char unmodeled_00[0x8];
    unsigned short value;       /* +0x08 */
};

int func_00A86CB0(Ov01Object *object) {
    object->flags &= ~1;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A86CC8);

void streamPlayByKey(int, int);

int unitCmdStreamPlay(int value) {
    seVoiceStartTagged(2, 2);
    streamPlayByKey(value, 3);
    return 1;
}

void unitRemove(void);

int unitCmdRemove(void) {
    unitRemove();
    return 1;
}

void unitRefreshPhase(Ov01Object *object);

int func_00A86DF0(Ov01Object *object) {
    /* mode 2 and mode 3 skip the call */
    if ((unsigned int) (object->motion->mode - 2) >= 2U) {
        unitRefreshPhase(object);
    }
    object->flags &= ~1;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A86E48);

void func_00A86F20(Ov01Object *object) {
    object->flags &= 0xFFFBFEFF;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A86F38);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A86FF0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A87030);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", objPoseSetSteps);

void slideHorizontalDistance(void *block);

void objPoseHorizontalDistance(Ov01Object *object) {
    slideHorizontalDistance(&object->poseSnapshotLow);
}

void slideLimitGoalXZ(void *block, f32 numerator);

/* Blends the pose snapshot toward its start by numerator (tail call; the
 * second argument passes through unchanged to slideLimitGoalXZ). */
void objPoseBlend(Ov01Object *object, f32 numerator) {
    slideLimitGoalXZ(&object->poseSnapshotLow, numerator);
}

int objStdMove(void *block);
void objPosePublish(Ov01Object *object);

/* One step of the pose snapshot, then the publish; returns the step's result. */
int objPoseStep(Ov01Object *object) {
    int arrived;

    arrived = objStdMove(&object->poseSnapshotLow);
    objPosePublish(object);
    return arrived;
}

float func_00A87390(int index);
short *func_00A9B3B8(int key);

/* unusedFirst keeps its argument slot; the body never reads it. */
float objPoseSlideLimit(int unusedFirst, int value, int key) {
    float base;

    if (value != 0) {
        base = func_00A87390(value);
    } else {
        base = 0.0f;
    }
    return base + ((float) func_00A9B3B8(key)[1] / 100.0f);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A87390);

/* Publishes the pose snapshot and angle to the source and to the motion. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu016", objPosePublish);

void func_0028B158(void *source, int first, int second, int third);

/* Loads the pose snapshot from the source and returns its low word. */
long long objPoseFetch(Ov01Object *object) {
    Ov01Source *source;
    long long snapshotLow;

    source = object->source;
    func_0028B158(source, 1, 0, 1);
    snapshotLow = source->snapshotLow;
    object->poseSnapshotLow = snapshotLow;
    object->poseSnapshotHigh = source->snapshotHigh;
    return snapshotLow;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A87508);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A876E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A87930);

void *dataLookupCurrent(int key);

/* Looks the object's key up; with action 1 it copies the entry's value into
 * the motion and runs unitRefreshPhase. */
void objMotionCopyLookup(Ov01Object *object, int action) {
    Ov01Lookup *entry;

    entry = dataLookupCurrent((int) &object->lookupKey);
    if (action == 1) {
        object->motion->copiedValue = entry->value;
        unitRefreshPhase(object);
    }
}

/* Heading from the snapshot to the pose's current point (atan2 of the x and
 * z deltas), wrapped, with motion->angle kept when the two points coincide. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu016", objPoseHeadingUpdate);

INCLUDE_ASM("asm/nonmatchings/ov01/tu016", func_00A87DB8);
