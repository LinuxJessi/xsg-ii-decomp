#include "common.h"
#include "main/pad.h"
#include "shared.h"
#include "main/tu021.h"
#include "main/tu051.h"
#include "main/tu075.h"
#include "main/xenoActor.h"
#include "main/xgl_font.h"

void func_A9CDB0(void *, void *);
extern unsigned char uwamonoNativeTable[];
extern unsigned char D_0069D310[];

/* Registers the uwamono native table uwamonoNativeTable with the module name D_0069D310. */
void uwamonoRegisterNatives(void) {
    func_A9CDB0(D_0069D310, uwamonoNativeTable);
}

int unduCheckAngle(f32, f32, f32);
extern f32 D_0069AF84;
extern f32 D_0069AF88;
extern f32 D_0069AF8C;
extern f32 D_0069AF90;

/*
 * Tests angle against four bounds loaded in the order D_0069AF84,
 * D_0069AF88, D_0069AF8C, D_0069AF90. Each test passes an adjacent pair of
 * bounds. Returns the index of the first test that holds, or -1 when none.
 * The four bounds are -3pi/4, 3pi/4, pi/4 and -pi/4, so the result is one of
 * four 90-degree compass sectors (printed as "Compass = %1d" by uwamonoLadderMove).
 */
int uwamonoCompassSector(f32 angle)
{
    f32 sectorEdge0;
    f32 sectorEdge1;
    f32 sectorEdge2;
    f32 sectorEdge3;

    sectorEdge0 = D_0069AF84;
    sectorEdge1 = D_0069AF88;
    if (unduCheckAngle(angle, sectorEdge1, sectorEdge0) != 0) {
        return 0;
    }
    sectorEdge2 = D_0069AF8C;
    if (unduCheckAngle(angle, sectorEdge2, sectorEdge1) != 0) {
        return 1;
    }
    sectorEdge3 = D_0069AF90;
    if (unduCheckAngle(angle, sectorEdge3, sectorEdge2) != 0) {
        return 2;
    }
    return (unduCheckAngle(angle, sectorEdge0, sectorEdge3) != 0) ? 3 : -1;
}

typedef struct LadderSlot {
    short id;                               /* 0x00 */
    unsigned char unmodeled_02[6];
    int key;                                /* 0x08: -1 when the slot holds no ladder */
    unsigned char unmodeled_0c[4];
    AlignedVector4 position;                /* 0x10: the foot of the ladder, y is its height */
    float angle;                            /* 0x20: facing angle of the ladder */
    float height;                           /* 0x24 */
    float rungs[32];                        /* 0x28: height of each rung above the foot */
    unsigned char rungExits[32];            /* 0xA8: RUNG_EXIT_LEFT and RUNG_EXIT_RIGHT bits */
    short rungCount;                        /* 0xC8 */
    signed char footBlocked;                /* 0xCA: 1 the foot of the ladder cannot be left */
    unsigned char unmodeled_cb[5];
} LadderSlot;

typedef struct LadderGameWork {
    unsigned char unmodeled_000[8];
    int flags;                              /* 0x008 */
    unsigned char exitPhase;                /* 0x00C: 2 once the parabolic exit lands */
    unsigned char unmodeled_00d[0x300 - 0xD];
    int climbFrames;                        /* 0x300 */
} LadderGameWork;

extern LadderSlot ladderSlots[];
extern short uwamonoLadderSlot;

/* The 8-byte ladder state block in .sdata: only the state is read or written in this TU.  As a
 * struct (not a plain short) its stores alias the actor loads, which orders the climb blocks. */
typedef struct LadderStateWork {
    short state;                            /* 0x0: 4 on the ladder, 5/6 climbing, 7 leaving, 8 top */
    short unused[3];
} LadderStateWork;

extern LadderStateWork uwamonoLadderState;
extern LadderGameWork gameWork;
extern PadRecord PadData;

#define LADDER_PI 3.1415927f
#define LADDER_HALF_PI 1.5707964f
/* LadderStateWork.state */
enum {
    LADDER_STATE_NONE = 0,
    LADDER_STATE_ON = 4,
    LADDER_STATE_CLIMB_UP = 5,
    LADDER_STATE_CLIMB_DOWN = 6,
    LADDER_STATE_EXIT = 7,
    LADDER_STATE_TOP = 8
};
/* LadderSlot.rungExits */
enum {
    RUNG_EXIT_LEFT = 1,
    RUNG_EXIT_RIGHT = 2
};
/* Ladder slots in ladderSlots (tu076.c fills them). */
enum {
    LADDER_SLOT_COUNT = 16
};
/* uwamonoLadderApproach counts LADDER_APPROACH_FRAMES frames before it turns to the ladder; the foot exit
 * (LADDER_FOOT_EXIT_FRAMES) is set by uwamonoLadderTryExit. */
enum {
    LADDER_APPROACH_FRAMES = 7,
    LADDER_FOOT_EXIT_FRAMES = 0x20
};

/* Ladder climb state (names are the scaffold's data labels). */
extern short ladderRungIndex;
/* index of the rung the climb started from */
extern float ladderClimbFrom;
/* height at the start of the current rung step */
extern float ladderClimbTo;
/* height at the end of the current rung step */
extern short ladderClimbFrame;
/* frames elapsed in the current rung step */
extern signed char ladderExitSide;
/* 1 when leaving sideways from a rung, 0 at the foot */
extern short ladderFrame;
/* cleared when the exit starts */
extern short ladderExitFrames;
/* frames of the exit motion (0x20 foot, 0x2A side) */
extern float ladderExitOffset;
/* height offset applied during the exit (-0.13 near a rung top) */
extern float ladderRungHeights[];
/* rung step heights, read at the current rung index */
extern AlignedVector4 ladderExitPoint;
/* where the exit moves the player to */
void uwamonoSetMotion(int index, int motion, float speed);
void uwamonoSendEvent(int kind, int id, int value);
void cameraApplyAllSlots(int arg);
float unduGetSlotHeight(int index, Vector4 *point, int arg2, int arg3, int arg4);
void func_0028B060(struct XenoActor *actor);
float sinf(float angle);
float cosf(float angle);
void Get_MiddlePoint(const Vector4 *first, const Vector4 *second, int current, int total, Vector4 *destination);
void XenoActorSetFirstWord(struct XenoActor *actor, int word);
extern float ladderFootHeight;
extern float ladderStepHeights[];
extern const char D_006899D0[];
extern f32 D_0069AF94;

void uwamonoLadderApproach(void)
{
    UwamonoObject *object = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = object->actor;
    short stepIndex;
    int nextFrame;
    short state = uwamonoLadderState.state;
    short mirror = state;
    short exitSide;

    if (state == 2) {
        int nextFrame;

        xglFontDebugPrintf(8, 100, D_006899D0);
        nextFrame = (unsigned short)ladderFrame + 1;
        ladderFrame = nextFrame;
        if ((short)nextFrame < LADDER_APPROACH_FRAMES) {
            return;
        }
        ladderFrame = 0;
        actor->direction.y = ladderSlots[uwamonoLadderSlot].angle;
        exitSide = ladderExitSide;
        if (exitSide != 0) {
            float angle;

            int motion = 19;
            motion = (exitSide ^ state) ? motion : 18;
            uwamonoSetMotion(uwamonoCurrentIndex, motion, 1.0f);
            angle = ladderSlots[uwamonoLadderSlot].angle;
            actor->direction.y = angle;
            actor->rotation.y = angle;
            XenoActorSetFirstWord(actor, 15);
            cameraApplyAllSlots(0);
        } else {
            uwamonoSetMotion(uwamonoCurrentIndex, 13, 1.0f);
            XenoActorSetFirstWord(actor, 15);
        }
        uwamonoLadderState.state = 3;
    }

    Get_MiddlePoint(&object->moveFrom, &object->moveTo, ladderFrame, ladderExitFrames, &actor->position);
    nextFrame = (unsigned short)ladderFrame + 1;
    if (ladderFrame < ladderExitFrames) {
        goto incr;
    }
    {
        DoublewordAlignedVector4 landingPoint;

        landingPoint = actor->position;
        ladderFrame = -1;
        uwamonoLadderState.state = LADDER_STATE_ON;
        unduGetSlotHeight(uwamonoCurrentIndex, &landingPoint, 256, -1, 0);
        ladderRungIndex = 63;
        if (ladderExitSide == 0) {
            actor->position.y = ladderFootHeight;
            if (mirror != state) {
                actor->position.x = ladderFootHeight;
            }
        }
        ladderStepHeights[63] = actor->position.y;
        if (ladderExitSide == 0) {
            cameraApplyAllSlots(0);
        }
        uwamonoSetMotion(uwamonoCurrentIndex, 20, 1.0f);
        if (ladderExitSide == 0) {
            XenoActorSetFirstWord(actor, 20);
        }
        XenoActorApplySourceValue(actor);
        actor->flags |= 1;
        for (stepIndex = 1; stepIndex < 63; stepIndex++) {
            float center = actor->position.y;

            ladderStepHeights[63 + stepIndex] = center + (float)stepIndex * D_0069AF94;
            ladderStepHeights[63 - stepIndex] = center - (float)stepIndex * D_0069AF94;
        }
    }
    goto done;
incr:
    ladderFrame = nextFrame;
done:
    func_0028B060(actor);
}

void Get_MiddlePoint_Parabora(const Vector4 *first, const Vector4 *second, int current, int total,
                              float height, Vector4 *destination);
void XenoActorShiftJoints(struct XenoActor *actor, int, int, int, int);
void XenoActorSetChannelValue(struct XenoActor *actor, float value);

/* The actor seen from its landing flags: hitFlags is read and written as a bit-field of this view,
 * which also holds the position, so the flag accesses alias the position copy and not the shorts. */
typedef struct ActorLandingView {
    int unmodeled_000[4];
    AlignedVector4 position;                /* 0x010 */
    int unmodeled_020[(0x748 - 0x20) / 4];
    unsigned short hitFlags : 16;           /* 0x748 */
} ActorLandingView;

void uwamonoParabolaMove(void)
{
    UwamonoObject *object = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = object->actor;
    ActorLandingView *landing = (ActorLandingView *) actor;
    short *exitFrame = &ladderFrame;

    ladderFrame = ladderFrame + 1;
    Get_MiddlePoint_Parabora(&object->moveFrom, &object->moveTo, ladderFrame, ladderExitFrames,
                             ladderExitOffset, &actor->position);
    if (ladderFrame >= ladderExitFrames) {
        unsigned short oldHitFlags = landing->hitFlags;

        actor->position = ladderExitPoint;
        *exitFrame = -1;
        uwamonoLadderState.state = 0;
        actor->position.y = object->moveTo.y;
        landing->hitFlags = 0x1000 | oldHitFlags;
        XenoActorSetChannelValue(actor, 4.0f);
        uwamonoSetMotion(uwamonoCurrentIndex, 0, 1.0f);
        XenoActorShiftJoints(actor, -1, 1, 0, 1);
        if (ladderExitSide != 0) {
            cameraApplyAllSlots(0);
        }
        gameWork.exitPhase = 2;
    }
    func_0028B060(actor);
}

extern const char D_006899E0[];
extern const char D_006899F0[];
extern const char D_00689A00[];
float Get_Distance(const Vector4 *a, const Vector4 *b);

/*
 * Tries to start leaving the ladder slot at index from the player's position: from the foot when the
 * player is within 0.7 of it facing the ladder, or sideways from a rung when the player faces the
 * 60 to 120 degree sector on the matching side. Returns 1 and starts the exit when one applies.
 */
int uwamonoLadderTryExit(short index)
{
    LadderSlot *slot = &ladderSlots[index];
    UwamonoObject *object = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = object->actor;
    int leaving = 0;
    int rungIndex;
    float footDistance;
    float leftDistance;
    float rightDistance;
    float xPlusSq, xPlusA, xPlusB, zPlusA, zPlusB, xMinusSq, xMinusA, xMinusB, zMinusA, zMinusB;

    if (slot->key != -1) {
        goto body;
    }
    return 0;

footExit:
    object->moveFrom = actor->position;
    object->moveTo = slot->position;
    ladderFootHeight = object->moveTo.y + 0.4f;
    actor->direction.y = slot->angle;
    ladderExitFrames = LADDER_FOOT_EXIT_FRAMES;
    ladderExitSide = 0;
    goto finalize;

body:
    do {
    footDistance = Get_Distance(&actor->position, &slot->position);
    xPlusA = actor->position.x - (slot->position.x + sinf(slot->angle + 2.1816616f) * 0.87f);
    xPlusB = actor->position.x - (slot->position.x + sinf(slot->angle + 2.1816616f) * 0.87f);
    xPlusSq = xPlusA * xPlusB;
    zPlusA = actor->position.z - (slot->position.z + cosf(slot->angle + 2.1816616f) * 0.87f);
    zPlusB = actor->position.z - (slot->position.z + cosf(slot->angle + 2.1816616f) * 0.87f);
    leftDistance = __builtin_sqrtf(xPlusSq + zPlusA * zPlusB);
    xMinusA = actor->position.x - (slot->position.x + sinf(slot->angle - 2.1816616f) * 0.87f);
    xMinusB = actor->position.x - (slot->position.x + sinf(slot->angle - 2.1816616f) * 0.87f);
    xMinusSq = xMinusA * xMinusB;
    zMinusA = actor->position.z - (slot->position.z + cosf(slot->angle - 2.1816616f) * 0.87f);
    zMinusB = actor->position.z - (slot->position.z + cosf(slot->angle - 2.1816616f) * 0.87f);
    rightDistance = __builtin_sqrtf(xMinusSq + zMinusA * zMinusB);

    if (footDistance < 0.7f
        && unduCheckAngle(actor->rotation.y, slot->angle - 0.5235988f, slot->angle + 0.5235988f) == 1
        && slot->position.y - 0.05f <= actor->position.y
        && actor->position.y <= slot->position.y + 0.1f) {
        goto footExit;
    }

    for (rungIndex = 0; rungIndex < slot->rungCount; rungIndex++) {
        float rungY = slot->position.y + slot->rungs[rungIndex];

        if (actor->position.y < rungY - 0.3f || rungY + 0.1f < actor->position.y) {
            continue;
        }
        xglFontDebugPrintf(8, 138, D_006899E0, rungIndex);
        if (leftDistance < 0.75f && (slot->rungExits[rungIndex] & RUNG_EXIT_LEFT)) {
            xglFontDebugPrintf(8, 138, D_006899F0, rungIndex);
            if (unduCheckAngle(actor->direction.y, slot->angle - 2.0943952f, slot->angle - 1.0471976f) == 1) {
                object->moveTo.x = slot->position.x;
                object->moveTo.y = slot->position.y + slot->rungs[rungIndex];
                object->moveTo.z = slot->position.z;
                object->moveTo.w = 1.0f;
                leaving = 1;
                object->moveFrom = object->moveTo;
                object->moveFrom.y = actor->position.y;
                actor->direction.y = slot->angle - LADDER_HALF_PI;
                ladderExitFrames = 57;
                ladderExitSide = leaving;
                break;
            }
        }
        if (rightDistance < 0.75f && (slot->rungExits[rungIndex] & RUNG_EXIT_RIGHT)) {
            xglFontDebugPrintf(8, 138, D_00689A00, rungIndex);
            if (unduCheckAngle(actor->direction.y, slot->angle + 1.0471976f, slot->angle + 2.0943952f) == 1) {
                object->moveTo.x = slot->position.x;
                object->moveTo.y = slot->position.y + slot->rungs[rungIndex];
                object->moveTo.z = slot->position.z;
                object->moveTo.w = 1.0f;
                leaving = 1;
                object->moveFrom = object->moveTo;
                object->moveFrom.y = actor->position.y;
                actor->direction.y = slot->angle + LADDER_HALF_PI;
                ladderExitSide = 2;
                ladderExitFrames = 57;
                break;
            }
        }
    }
    if (leaving != 1) {
        return 0;
    }

finalize:
    uwamonoLadderState.state = 2;
    object->turnFrom.y = actor->direction.y;
    object->turnTo.y = slot->angle;
    uwamonoLadderSlot = index;
    ladderFrame = 0;
    XenoActorSetChannelValue(actor, 4.0f);
    uwamonoSetMotion(uwamonoCurrentIndex, 0, 1.0f);
    uwamonoLadderApproach();
    func_0028B060(actor);
    return 1;
    } while (0);
}

void uwamonoLadderMove(void)
{
    int exited;
    LadderSlot *slot = &ladderSlots[uwamonoLadderSlot];
    UwamonoObject *object = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = object->actor;
    int compass;
    float side;
    float climb;
    float stickX;
    float stickY;
    float x;
    float y;
    int i;
    AlignedVector4 exitPoint;
    AlignedVector4 target;

    exited = 0;
    compass = uwamonoCompassSector(slot->angle);
    side = 0.0f;
    climb = side; /* both start at zero; kept as a chained copy for the original code generation */
    if (!(gameWork.flags & CONTROL_SCRIPTED)) {
        stickX = PadData.directionX;
        stickY = PadData.directionY;
        if (stickX >= 32.0f || stickX <= -32.0f) {
            x = stickX * 0.0004f * 1.5f;
        } else {
            x = 0.0f;
        }
        if (stickY >= 32.0f || stickY <= -32.0f) {
            y = stickY * 0.0004f * 1.5f;
        } else {
            y = 0.0f;
        }
        if (x == 0.0f && y == 0.0f) {
            if (PadData.held & PAD_UP) {
                y = -1.5f;
                x = 0.0f;
            }
            if (PadData.held & PAD_DOWN) {
                y = 1.5f;
                x = 0.0f;
            }
            if (PadData.held & PAD_LEFT) {
                x = -1.5f;
                y = 0.0f;
            }
            if (PadData.held & PAD_RIGHT) {
                x = 1.5f;
                y = 0.0f;
            }
        }
        switch (compass) {
        case 0:
            side = x;
            climb = -y;
            break;
        case 1:
            side = y;
            climb = x;
            break;
        case 2:
            side = x;
            climb = y;
            break;
        case 3:
            side = -y;
            climb = -x;
            break;
        }
    }
    xglFontDebugPrintf(4, 0x80, "XYZ = %4f,%4f", side, climb);
    xglFontDebugPrintf(4, 0x8A, "Compass = %1d", compass);
    switch (uwamonoLadderState.state) {
    case LADDER_STATE_CLIMB_UP:
        ladderClimbFrame++;
        if (ladderClimbFrame >= gameWork.climbFrames) {
            uwamonoLadderState.state = LADDER_STATE_ON;
            actor->flags |= 1;
            XenoActorApplySourceValue(actor);
        }
        climb = ladderClimbFrom + (ladderClimbTo - ladderClimbFrom) * ladderClimbFrame / gameWork.climbFrames - actor->position.y;
        target = actor->position;
        target.y += 5.0f;
        uwamonoOfferTarget(uwamonoCurrentIndex, 1, -1, &target);
        break;
    case LADDER_STATE_CLIMB_DOWN:
        ladderClimbFrame++;
        if (ladderClimbFrame >= gameWork.climbFrames) {
            uwamonoLadderState.state = LADDER_STATE_ON;
            actor->flags |= 1;
            XenoActorApplySourceValue(actor);
        }
        climb = ladderClimbFrom + (ladderClimbTo - ladderClimbFrom) * ladderClimbFrame / gameWork.climbFrames - actor->position.y;
        /* The original adjusts target here without setting it first (only the climb-up case copies the position); kept as is. */
        target.y -= 5.0f;
        uwamonoOfferTarget(uwamonoCurrentIndex, 1, -1, &target);
        break;
    case LADDER_STATE_ON:
        ladderClimbFrame = 0;
        if (climb < 0.0f) {
            if (actor->position.y - 0.8f <= slot->position.y && slot->footBlocked == 1) {
                return;
            }
            seVoiceStartDefault(0x200000D);
            ladderClimbFrame = 1;
            ladderClimbFrom = actor->position.y;
            ladderClimbTo = ladderRungHeights[ladderRungIndex--];
            uwamonoLadderState.state = LADDER_STATE_CLIMB_DOWN;
            uwamonoSetMotion(uwamonoCurrentIndex, 0x14, -2.0f);
            XenoActorSetPlayRange(actor, 0, 0x1D);
            actor->flags &= ~1;
            climb = ladderClimbFrom + (ladderClimbTo - ladderClimbFrom) * ladderClimbFrame / gameWork.climbFrames - actor->position.y;
        }
        if (climb > 0.0f) {
            if (slot->position.y + slot->height <= actor->position.y + 0.8f) {
                uwamonoSendEvent(4, ladderSlots[uwamonoLadderSlot].id, 0);
                if (gameWork.flags & CONTROL_TOP_EXIT) {
                    uwamonoLadderState.state = LADDER_STATE_TOP;
                }
                return;
            }
            seVoiceStartDefault(0x200000D);
            ladderClimbFrame = 1;
            ladderClimbFrom = actor->position.y;
            ladderClimbTo = ladderRungHeights[ladderRungIndex++ + 2];
            uwamonoLadderState.state = LADDER_STATE_CLIMB_UP;
            uwamonoSetMotion(uwamonoCurrentIndex, 0x14, 2.0f);
            XenoActorSetPlayRange(actor, 0, 0x1D);
            actor->flags &= ~1;
            climb = ladderClimbFrom + (ladderClimbTo - ladderClimbFrom) * ladderClimbFrame / gameWork.climbFrames - actor->position.y;
        }
        break;
    }
    if (slot->position.y + slot->height < actor->position.y + climb) {
        actor->position.y = slot->position.y + slot->height;
        return;
    }
    if (actor->position.y + climb < slot->position.y + 0.4f && slot->footBlocked == 0) {
        exited = 1;
        uwamonoSendEvent(5, ladderSlots[uwamonoLadderSlot].id, 0);
        actor->position.y = slot->position.y;
        exitPoint = actor->position;
        exitPoint.x = actor->position.x + sinf(slot->angle - LADDER_PI) * 0.8f;
        exitPoint.y = actor->position.y;
        exitPoint.z = actor->position.z + cosf(slot->angle - LADDER_PI) * 0.8f;
        exitPoint.w = 1.0f;
        ladderExitPoint = exitPoint;
        ladderExitFrames = 0x20;
        ladderExitOffset = 0;
        ladderExitSide = 0;
        uwamonoSetMotion(uwamonoCurrentIndex, 0x11, 1.0f);
        actor->flags &= ~1;
        cameraApplyAllSlots(0);
    } else {
        actor->position.y = actor->position.y + climb;
        if (uwamonoLadderState.state == LADDER_STATE_ON) {
            for (i = 0; i < slot->rungCount; i++) {
                float rungY = slot->position.y + slot->rungs[i];

                if (actor->position.y < rungY - 0.3f || rungY + 0.45f < actor->position.y) {
                    continue;
                }
                if (side < 0.0f && (slot->rungExits[i] & RUNG_EXIT_LEFT)) {
                    exitPoint.x = actor->position.x;
                    exitPoint.y = actor->position.y;
                    exitPoint.z = actor->position.z;
                    exitPoint.w = 1.0f;
                    ladderExitPoint.x = actor->position.x + sinf(slot->angle + LADDER_HALF_PI) * 1.2f;
                    ladderExitPoint.y = actor->position.y;
                    ladderExitPoint.z = actor->position.z + cosf(slot->angle + LADDER_HALF_PI) * 1.2f;
                    ladderExitPoint.x = ladderExitPoint.x + sinf(slot->angle - LADDER_PI) * 0.5f;
                    ladderExitPoint.z = ladderExitPoint.z + cosf(slot->angle - LADDER_PI) * 0.5f;
                    ladderExitPoint.w = 1.0f;
                    ladderExitFrames = 0x2A;
                    ladderExitSide = 1;
                    exited = 1;
                    XenoActorSetChannelValue(actor, 4.0f);
                    if (slot->position.y + slot->rungs[i] + 0.3f <= actor->position.y) {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0x10, 1.0f);
                        ladderExitOffset = -0.13f;
                    } else {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0x10, 1.0f);
                        ladderExitOffset = 0;
                    }
                    actor->flags &= ~1;
                    break;
                }
                if (side > 0.0f && (slot->rungExits[i] & RUNG_EXIT_RIGHT)) {
                    exitPoint.x = actor->position.x;
                    exitPoint.y = actor->position.y;
                    exitPoint.z = actor->position.z;
                    exitPoint.w = 1.0f;
                    ladderExitPoint.x = actor->position.x + sinf(slot->angle - LADDER_HALF_PI) * 1.2f;
                    ladderExitPoint.y = actor->position.y;
                    ladderExitPoint.z = actor->position.z + cosf(slot->angle - LADDER_HALF_PI) * 1.2f;
                    ladderExitPoint.x = ladderExitPoint.x + sinf(slot->angle - LADDER_PI) * 0.5f;
                    ladderExitPoint.z = ladderExitPoint.z + cosf(slot->angle - LADDER_PI) * 0.5f;
                    ladderExitPoint.w = 1.0f;
                    ladderExitFrames = 0x2A;
                    ladderExitSide = 1;
                    exited = 1;
                    XenoActorSetChannelValue(actor, 4.0f);
                    if (slot->position.y + slot->rungs[i] + 0.3f <= actor->position.y) {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0xF, 1.0f);
                        ladderExitOffset = -0.13f;
                    } else {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0xF, 1.0f);
                        ladderExitOffset = 0;
                    }
                    actor->flags &= ~1;
                    break;
                }
            }
        }
        if (exited == 1) {
            exitPoint.y = unduGetSlotHeight(uwamonoCurrentIndex, &ladderExitPoint, 0x100, -1, 0);
        }
    }
    if (exited == 1) {
        object->moveFrom = actor->position;
        object->moveTo = exitPoint;
        ladderFrame = 0;
        uwamonoLadderState.state = LADDER_STATE_EXIT;
    }
    func_0028B060(actor);
}

extern const char D_00689A30[];
void uwamonoHaltPursuit(int);
float cursolGetByReduceSpeedAngleLoop(float, float, float);
/* One 0x870-byte entry of uwamonoSlotTable (tu074.c): the actor body pointer is at +0x00. */
typedef struct SlotEntry {
    struct XenoActor *actor;
    unsigned char unmodeled_04[0x870 - 0x04];
} SlotEntry;

extern SlotEntry uwamonoSlotTable[];

int uwamonoUpdateLadderState(void)
{
    struct XenoActor *actor = uwamonoSlotTable[uwamonoCurrentIndex].actor;
    int slotIndex;

    if (uwamonoLadderState.state == LADDER_STATE_NONE) {
        for (slotIndex = 0; slotIndex < LADDER_SLOT_COUNT; slotIndex++) {
            if (ladderSlots[slotIndex].id == -1) {
                continue;
            }
            if (uwamonoLadderTryExit(slotIndex) != 0) {
                uwamonoHaltPursuit(-1);
                break;
            }
        }
        if (uwamonoLadderState.state == LADDER_STATE_NONE) {
            return 0;
        }
    }
    xglFontDebugPrintf(8, 12, D_00689A30);
    actor->rotation.y = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 6.0f);
    switch (uwamonoLadderState.state) {
    case 1:
        break;
    case 2:
    case 3:
        uwamonoLadderApproach();
        break;
    case LADDER_STATE_ON:
    case LADDER_STATE_CLIMB_UP:
    case LADDER_STATE_CLIMB_DOWN:
        uwamonoLadderMove();
        break;
    case LADDER_STATE_EXIT:
        uwamonoParabolaMove();
        break;
    }
    return 1;
}
