/*
 * OV01 original TU 12: 0x00a84830..0x00a85a80 (37 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/xenoActor.h"
#include "ov01/tu005.h"
#include "ov01/tu012.h"
#include "ov01/tu033.h"

typedef struct Ov01Pose {
    long long snapshot[2]; /* +0x0: vector words; unitUpdate copies Ov01Unit.snapshot here */
    unsigned char unmodeled_10[0x14];
    f32 angle; /* +0x24: radians, see unitSetAngleRadians */
} Ov01Pose;

typedef struct Ov01Motion {
    unsigned char unmodeled_00[0x14];
    signed char state;
    unsigned char unmodeled_15[0x4D];
    u16 mode;
    unsigned char unmodeled_64[0x8];
    f32 angle;
    unsigned char unmodeled_70[0xA];
    u16 slot_flags[3];
} Ov01Motion;

typedef struct Ov01Unit {
    unsigned char unmodeled_00[0x10];
    long long snapshot[2];
    unsigned char unmodeled_20[0x9C];
    u32 flags;
    unsigned char unmodeled_c0[0x50C];
    u32 status;
} Ov01Unit;

typedef struct Ov01Object {
    unsigned char unmodeled_00[0xC];
    u32 flags;
    unsigned char unmodeled_10[0x134];
    Ov01Motion *motion;
    unsigned char unmodeled_148[0x8];
    Ov01Pose pose;
    unsigned char unmodeled_178[0x58];
    Ov01Unit *unit;
    unsigned char unmodeled_1d4[0x8];
    Ov01Unit *children[3];
} Ov01Object;

void unitRunCommands(void *);
void func_00A95680(void *);

/* Per-frame step: clears flag 0x20000, runs the command queue, copies the unit's snapshot into the pose, then calls func_00A95680. */
void unitUpdate(Ov01Object *object) {
    object->flags &= ~0x00020000u;
    unitRunCommands(object);
    object->pose.snapshot[0] = object->unit->snapshot[0];
    object->pose.snapshot[1] = object->unit->snapshot[1];
    func_00A95680(object);
}

/*
 * Ov01Owner: the object whose +0x1D0 holds its unit. Ov01Object has the same
 * pointer at that offset; the functions below read only that pointer.
 * Ov01UnitTail: the unit's record from +0x5D4 on. Its int count is read by
 * func_00A849B8 (which returns count + 1) and compared by func_00A849C8.
 * Bytes between the evidenced members are untouched here.
 */
typedef struct Ov01UnitTail Ov01UnitTail;

typedef struct Ov01Owner Ov01Owner;

struct Ov01UnitTail {
    unsigned char unmodeled_00[0x5D4];
    int count; /* +0x5D4 */
};

struct Ov01Owner {
    unsigned char unmodeled_00[0x1D0];
    Ov01UnitTail *unit; /* +0x1D0 */
};

void func_00A84880(void) {

}

void unitClearBindFlags(Ov01Object *object);

void unitUnbind(Ov01Object *object) {
    unitClearBindFlags(object);
    func_00A955C8(object);
}

/* Clears bits 0x10004 of the unit and of each child whose slot flag 0x4000 is clear, and bit 2 of the object. */
void unitClearBindFlags(Ov01Object *object) {
    Ov01Unit *child;
    int i;

    object->unit->flags &= ~0x00010004u;
    object->flags &= ~0x00000002u;
    for (i = 0; i < 3; i++) {
        child = object->children[i];
        if (child != 0 && !(object->motion->slot_flags[i] & 0x4000)) {
            child->flags &= ~0x00010004u;
        }
    }
}

void unitSetBindFlags(Ov01Object *object);
void func_00A95608(void *owner);

void unitBindSetup(Ov01Object *object) {
    unitSetBindFlags(object);
    func_00A95608(object);
}

/* Sets bit 4 of the unit and of each child, and bit 2 of the object: the mirror of unitClearBindFlags. */
void unitSetBindFlags(Ov01Object *object) {
    Ov01Unit *child;
    int i;

    object->unit->flags |= 4;
    object->flags |= 2;
    for (i = 0; i < 3; i++) {
        child = object->children[i];
        if (child != 0) {
            child->flags |= 4;
        }
    }
}

/* Returns the unit's count plus one; func_00A849C8 compares that with 2 and 0xB. */
int func_00A849B8(Ov01Owner *owner) {
    return owner->unit->count + 1;
}

/* Phase setter: the callers pass fixed phase numbers, and it updates the unit's status bit 2 and its children. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A849C8);

/* Does nothing for value 0; else sets status bit 2 on the unit and calls the timing helpers on it and on children with 0x600 set. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A84B68);

f32 XenoActorSetSelector(void *, int);

/* For motion modes 1 to 10: the unit's timing call with 7.0f, then XenoActorSetSelector with the selector. */
void unitSetSelector(Ov01Object *object, int selector) {
    Ov01Unit *unit;

    if ((unsigned int) (object->motion->mode - 1) < 10) {
        unit = object->unit;
        if (!(unit->status & 0x200)) {
            XenoActorSetChannelValueB(unit, 7.0f);
            XenoActorEnableChannelB(unit);
        }
        XenoActorSetSelector(unit, selector);
    }
}

void func_00A849C8(void *, int);
int func_00A907B0(void);

/* Picks phase 2 with selector 6 when func_00A907B0 holds and the mode is 1 to 10, else phase 1 with selector 0. */
void unitRefreshPhase(Ov01Object *object) {
    int phase;
    int selector;

    if ((func_00A907B0() != 0) && ((unsigned int) (object->motion->mode - 1) < 10)) {
        phase = 2;
        selector = 6;
    } else {
        phase = 1;
        selector = 0;
    }
    func_00A849C8(object, phase);
    unitSetSelector(object, selector);
}

void XenoActorSetVector(void *child, float, float, float);

/* Calls XenoActorSetVector on the unit with x and y scale 1.0 and the third component scaleZ. */
void unitSetVectorZ(Ov01Owner *owner, float scaleZ) {
    XenoActorSetVector(owner->unit, 1.0f, 1.0f, scaleZ);
}


/* Returns XenoActorIsDone's result: the state handlers test it as a done flag. */
int unitIsDone(Ov01Owner *owner) {
    return XenoActorIsDone(owner->unit);
}

int func_00264A80(int key);

/* The key is keyHigh << 12 | keyLow; the unused first argument is kept for the calling convention. */
int func_00A84D80(int unused, int keyHigh, int keyLow) {
    return func_00264A80((keyHigh << 0xC) | keyLow) != 0;
}

/* Loads the snapshot vector from the table entry for id (three ints divided by 100) and records id in the motion. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", unitLoadSnapshot);

void unitSetAngleRadians(Ov01Object *object, f32 angle);

/* Degrees to radians (pi / 180), then unitSetAngleRadians. */
void unitSetAngleDegrees(Ov01Object *object, int degrees) {
    unitSetAngleRadians(object, (f32) degrees * 0.017453292f);
}

void objPosePublish(void);

/* Stores the angle in radians, wrapped to [0, 2 pi), in the pose and in the motion. */
void unitSetAngleRadians(Ov01Object *object, f32 angle) {
    Ov01Pose *pose;

    pose = &object->pose;
    pose->angle = angle;
    if (angle < 0.0f) {
        pose->angle = angle + 6.2831855f;
    }
    objPosePublish();
    object->motion->angle = pose->angle;
}

/* The motion angle in degrees (radians times 180 / pi), truncated; 0 for a null object. */
int unitGetAngleDegrees(Ov01Object *object) {
    if (object == 0) {
        return 0;
    }
    return (int) (object->motion->angle * 57.295776f);
}

/* Zeroes the snapshot's first three words, then calls objPosePublish. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", unitClearSnapshot);

/* Speed factor by motion state: 2.0 for state 3, 3.0 for state 4, else 1.0. */
f32 unitStateScale(Ov01Object *object) {
    if (object->motion->state == 3) {
        return 2.0f;
    }
    if (object->motion->state == 4) {
        return 3.0f;
    }
    return 1.0f;
}

/*
 * With a target: heading = atan2 of the pose-minus-target x and z offsets,
 * plus pi when the flag is zero, wrapped to [0, 2 pi); stored in the pose,
 * the unit's +0x54 and the motion. Then a point from the unit's matrix is
 * written to pose+0x60 (xglVectorMulMat on a vector built from unitStateScale).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", unitFaceTarget);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A85098);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A85118);

void dataLookupAppend(int value);

/* objectAddr is the Ov01Object address; its +0x10 is the command queue. The asm also forwards a second argument this prototype does not declare. */
void unitQueueAppend(int objectAddr) {
    dataLookupAppend(objectAddr + 0x10);
}


/* Empties the command queue at +0x10 of the object. */
void unitQueueClear(int objectAddr) {
    dataLookupReset(objectAddr + 0x10);
}

int dataLookupCurrent(int value);

/* Nonzero when the command queue at +0x10 of the object has no pending entry. */
int unitQueueIsEmpty(int objectAddr) {
    return dataLookupCurrent(objectAddr + 0x10) == 0;
}

/*
 * Runs the object's command queue. Each entry starts with a type below 0x28
 * that indexes the handler table unitCommandTable; a handler returning nonzero
 * retires the entry and resets the wait counter at +0x138, a zero return
 * keeps it and counts.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", unitRunCommands);

/* Handler table unitCommandTable entry 0: no operation, always done. */
int unitCmdNoop(void) {
    return 1;
}

/* Handler table unitCommandTable entry 1. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A852B8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A853A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A855D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A856B8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A85788);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A85848);

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A85910);

void unitRefreshPhase(Ov01Object *object);

/* Handler table unitCommandTable entry 8: calls unitRefreshPhase, then done. */
int unitCmdRefreshPhase(Ov01Object *object) {
    unitRefreshPhase(object);
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu012", func_00A859F8);
