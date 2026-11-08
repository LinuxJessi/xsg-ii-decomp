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

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoLadderApproach);

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoParabolaMove);

INCLUDE_ASM("asm/main/nonmatchings/tu075", func_001F8608);

/* One 208-byte record of the 16-slot ladder table at ladderSlots. */
typedef struct LadderSlot {
    short id;                               /* 0x00 */
    unsigned char unmodeled_02[0x12];
    float baseY;                            /* 0x14: height of the ladder foot */
    unsigned char unmodeled_18[8];
    float angle;                            /* 0x20: facing angle of the ladder */
    float height;                           /* 0x24 */
    float rungs[32];                        /* 0x28: height of each rung above the foot */
    unsigned char rungExits[32];            /* 0xA8: 1 the rung can be left to the left, 2 to the right */
    short rungCount;                        /* 0xC8 */
    signed char footBlocked;                /* 0xCA: 1 the foot of the ladder cannot be left */
    unsigned char unmodeled_cb[5];
} LadderSlot;

typedef struct LadderGameWork {
    unsigned char unmodeled_000[8];
    int flags;                              /* 0x008 */
    unsigned char unmodeled_00c[0x300 - 0xC];
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
#define LADDER_STATE_ON 4
#define LADDER_STATE_CLIMB_UP 5
#define LADDER_STATE_CLIMB_DOWN 6
#define LADDER_STATE_EXIT 7
#define LADDER_STATE_TOP 8

/* LadderSlot.rungExits */
#define RUNG_EXIT_LEFT 1
#define RUNG_EXIT_RIGHT 2


/* Ladder climb state (names are the scaffold's data labels). */
extern short D_0069E1C8;                /* index of the rung the climb started from */
extern float D_0069E1CC;                /* height at the start of the current rung step */
extern float D_0069E1D0;                /* height at the end of the current rung step */
extern short D_0069E1D8;                /* frames elapsed in the current rung step */
extern signed char D_0069E1DC;          /* 1 when leaving sideways from a rung, 0 at the foot */
extern short D_0069E1DE;                /* cleared when the exit starts */
extern short D_0069E1E0;                /* frames of the exit motion (0x20 foot, 0x2A side) */
extern float D_0069E1E4;                /* height offset applied during the exit (-0.13 near a rung top) */
extern float D_006A1B2C[];              /* rung step heights, read at the current rung index */
extern AlignedVector4 D_006A1D30;       /* where the exit moves the player to */

void uwamonoSetMotion(int index, int motion, float speed);
void func_00207650(int kind, int id, int value);
void cameraApplyAllSlots(int arg);
float func_0020A968(int index, Vector4 *point, int arg2, int arg3, int arg4);
void func_0028B060(struct XenoActor *actor);
float sinf(float angle);
float cosf(float angle);

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
        D_0069E1D8++;
        if (D_0069E1D8 >= gameWork.climbFrames) {
            uwamonoLadderState.state = LADDER_STATE_ON;
            actor->flags |= 1;
            XenoActorApplySourceValue(actor);
        }
        climb = D_0069E1CC + (D_0069E1D0 - D_0069E1CC) * D_0069E1D8 / gameWork.climbFrames - actor->position.y;
        target = actor->position;
        target.y += 5.0f;
        uwamonoOfferTarget(uwamonoCurrentIndex, 1, -1, &target);
        break;
    case LADDER_STATE_CLIMB_DOWN:
        D_0069E1D8++;
        if (D_0069E1D8 >= gameWork.climbFrames) {
            uwamonoLadderState.state = LADDER_STATE_ON;
            actor->flags |= 1;
            XenoActorApplySourceValue(actor);
        }
        climb = D_0069E1CC + (D_0069E1D0 - D_0069E1CC) * D_0069E1D8 / gameWork.climbFrames - actor->position.y;
        /* The original adjusts target here without setting it first (only the climb-up case copies the position); kept as is. */
        target.y -= 5.0f;
        uwamonoOfferTarget(uwamonoCurrentIndex, 1, -1, &target);
        break;
    case LADDER_STATE_ON:
        D_0069E1D8 = 0;
        if (climb < 0.0f) {
            if (actor->position.y - 0.8f <= slot->baseY && slot->footBlocked == 1) {
                return;
            }
            seVoiceStartDefault(0x200000D);
            D_0069E1D8 = 1;
            D_0069E1CC = actor->position.y;
            D_0069E1D0 = D_006A1B2C[D_0069E1C8--];
            uwamonoLadderState.state = LADDER_STATE_CLIMB_DOWN;
            uwamonoSetMotion(uwamonoCurrentIndex, 0x14, -2.0f);
            XenoActorSetPlayRange(actor, 0, 0x1D);
            actor->flags &= ~1;
            climb = D_0069E1CC + (D_0069E1D0 - D_0069E1CC) * D_0069E1D8 / gameWork.climbFrames - actor->position.y;
        }
        if (climb > 0.0f) {
            if (slot->baseY + slot->height <= actor->position.y + 0.8f) {
                func_00207650(4, ladderSlots[uwamonoLadderSlot].id, 0);
                if (gameWork.flags & CONTROL_TOP_EXIT) {
                    uwamonoLadderState.state = LADDER_STATE_TOP;
                }
                return;
            }
            seVoiceStartDefault(0x200000D);
            D_0069E1D8 = 1;
            D_0069E1CC = actor->position.y;
            D_0069E1D0 = D_006A1B2C[D_0069E1C8++ + 2];
            uwamonoLadderState.state = LADDER_STATE_CLIMB_UP;
            uwamonoSetMotion(uwamonoCurrentIndex, 0x14, 2.0f);
            XenoActorSetPlayRange(actor, 0, 0x1D);
            actor->flags &= ~1;
            climb = D_0069E1CC + (D_0069E1D0 - D_0069E1CC) * D_0069E1D8 / gameWork.climbFrames - actor->position.y;
        }
        break;
    }
    if (slot->baseY + slot->height < actor->position.y + climb) {
        actor->position.y = slot->baseY + slot->height;
        return;
    }
    if (actor->position.y + climb < slot->baseY + 0.4f && slot->footBlocked == 0) {
        exited = 1;
        func_00207650(5, ladderSlots[uwamonoLadderSlot].id, 0);
        actor->position.y = slot->baseY;
        exitPoint = actor->position;
        exitPoint.x = actor->position.x + sinf(slot->angle - LADDER_PI) * 0.8f;
        exitPoint.y = actor->position.y;
        exitPoint.z = actor->position.z + cosf(slot->angle - LADDER_PI) * 0.8f;
        exitPoint.w = 1.0f;
        D_006A1D30 = exitPoint;
        D_0069E1E0 = 0x20;
        D_0069E1E4 = 0;
        D_0069E1DC = 0;
        uwamonoSetMotion(uwamonoCurrentIndex, 0x11, 1.0f);
        actor->flags &= ~1;
        cameraApplyAllSlots(0);
    } else {
        actor->position.y = actor->position.y + climb;
        if (uwamonoLadderState.state == LADDER_STATE_ON) {
            for (i = 0; i < slot->rungCount; i++) {
                float rungY = slot->baseY + slot->rungs[i];

                if (actor->position.y < rungY - 0.3f || rungY + 0.45f < actor->position.y) {
                    continue;
                }
                if (side < 0.0f && (slot->rungExits[i] & RUNG_EXIT_LEFT)) {
                    exitPoint.x = actor->position.x;
                    exitPoint.y = actor->position.y;
                    exitPoint.z = actor->position.z;
                    exitPoint.w = 1.0f;
                    D_006A1D30.x = actor->position.x + sinf(slot->angle + LADDER_HALF_PI) * 1.2f;
                    D_006A1D30.y = actor->position.y;
                    D_006A1D30.z = actor->position.z + cosf(slot->angle + LADDER_HALF_PI) * 1.2f;
                    D_006A1D30.x = D_006A1D30.x + sinf(slot->angle - LADDER_PI) * 0.5f;
                    D_006A1D30.z = D_006A1D30.z + cosf(slot->angle - LADDER_PI) * 0.5f;
                    D_006A1D30.w = 1.0f;
                    D_0069E1E0 = 0x2A;
                    D_0069E1DC = 1;
                    exited = 1;
                    XenoActorSetChannelValue(actor, 4.0f);
                    if (slot->baseY + slot->rungs[i] + 0.3f <= actor->position.y) {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0x10, 1.0f);
                        D_0069E1E4 = -0.13f;
                    } else {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0x10, 1.0f);
                        D_0069E1E4 = 0;
                    }
                    actor->flags &= ~1;
                    break;
                }
                if (side > 0.0f && (slot->rungExits[i] & RUNG_EXIT_RIGHT)) {
                    exitPoint.x = actor->position.x;
                    exitPoint.y = actor->position.y;
                    exitPoint.z = actor->position.z;
                    exitPoint.w = 1.0f;
                    D_006A1D30.x = actor->position.x + sinf(slot->angle - LADDER_HALF_PI) * 1.2f;
                    D_006A1D30.y = actor->position.y;
                    D_006A1D30.z = actor->position.z + cosf(slot->angle - LADDER_HALF_PI) * 1.2f;
                    D_006A1D30.x = D_006A1D30.x + sinf(slot->angle - LADDER_PI) * 0.5f;
                    D_006A1D30.z = D_006A1D30.z + cosf(slot->angle - LADDER_PI) * 0.5f;
                    D_006A1D30.w = 1.0f;
                    D_0069E1E0 = 0x2A;
                    D_0069E1DC = 1;
                    exited = 1;
                    XenoActorSetChannelValue(actor, 4.0f);
                    if (slot->baseY + slot->rungs[i] + 0.3f <= actor->position.y) {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0xF, 1.0f);
                        D_0069E1E4 = -0.13f;
                    } else {
                        uwamonoSetMotion(uwamonoCurrentIndex, 0xF, 1.0f);
                        D_0069E1E4 = 0;
                    }
                    actor->flags &= ~1;
                    break;
                }
            }
        }
        if (exited == 1) {
            exitPoint.y = func_0020A968(uwamonoCurrentIndex, &D_006A1D30, 0x100, -1, 0);
        }
    }
    if (exited == 1) {
        object->exitFrom = actor->position;
        object->exitTo = exitPoint;
        D_0069E1DE = 0;
        uwamonoLadderState.state = LADDER_STATE_EXIT;
    }
    func_0028B060(actor);
}

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoUpdateLadderState);
