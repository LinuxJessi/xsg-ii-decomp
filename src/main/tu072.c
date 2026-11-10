#include "common.h"
#include "shared.h"
#include "main/tu072.h"
#include "main/xenoActor.h"

#define NULL ((void *) 0)

/* Script native argument access: the VM supplies the argument count and the
   integer argument at each 1-based position of the running native call. */
int func_A9F0A8(void);
void func_A9CC70(int call, int error_code);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);
void func_00208B28(void);
void func_00208B50(int value);
void vibrationStartFade(unsigned char targetValue, unsigned char currentValue, int framesTotal);

/* The XenoActor of an enepc record (+0x330); only the fields used below. */
struct XenoActor {
    unsigned char unmodeled_000[0xB0];
    float radius;                           /* +0x0B0 */
    unsigned char unmodeled_0b4[0x14C - 0xB4];
    float actorParam14C;                    /* +0x14C */
    unsigned char unmodeled_150[0x170 - 0x150];
    float actorParam170;                    /* +0x170 */
    float actorParam174;                    /* +0x174 */
    unsigned char unmodeled_178[0x5CC - 0x178];
    unsigned int flags;                     /* +0x5CC: 0x2 loops the motion */
    float playStart;                        /* +0x5D0 */
    int mode;                               /* +0x5D4: motion number within the group */
    int modeGroup;                          /* +0x5D8: motion number / 100 */
};

/* A path point: a quadword vector filled coordinate by coordinate (x, y, z).  The
   union keeps the original's ordering of the point stores against the script-array
   loads and the pointCount reloads, and the indexed coordinates give the original's
   single store cursor (set at z) in the copy loop. */
typedef union EnepcPoint {
    float coords[4];
    AlignedVector4 vector;
} EnepcPoint;

/* The state block of an enepc record (+0x334). */
typedef struct EnepcState {
    unsigned char unmodeled_0000[0x34];
    float floatParams[4];                   /* +0x0034 */
    float angle;                            /* +0x0044: radians */
    float floatParams48[2];                 /* +0x0048 */
    short shortParams[4];                   /* +0x0050 */
    unsigned char unmodeled_0058[7];
    unsigned char routeMode;                /* +0x005F: 9 is turned into 8 once the path is mirrored */
    unsigned char unmodeled_0060[0x10A0 - 0x60];
    EnepcPoint points[0x100];               /* +0x10A0: path points, script units / 100 */
    short pointLinks[0x100][4];             /* +0x20A0: -1 for none */
    short pointLinkValues[0x100][4];        /* +0x28A0 */
    unsigned char unmodeled_30a0[6];
    short pointCount;                       /* +0x30A6 */
    unsigned char unmodeled_30a8[0x37B0 - 0x30A8];
    short routeNodeCount;                   /* +0x37B0: counted up by uwamonoAddRouteNode */
    unsigned char unmodeled_37b2[0x3FF0 - 0x37B2];
    int flags;                              /* +0x3FF0 */
} EnepcState;

/* One 0x870-byte record of uwamonoRecords; only the fields used here. */
typedef struct EnepcRecord {
    unsigned char unmodeled_000[2];
    unsigned char kind;                     /* +0x002: 1/4 actor */
    unsigned char unmodeled_003[0x330 - 3];
    struct XenoActor *actor;                /* +0x330 */
    EnepcState *state;                      /* +0x334 */
    unsigned char unmodeled_338[4];
    short motion;                           /* +0x33C */
    unsigned char unmodeled_33e[0x580 - 0x33E];
    float extent;                           /* +0x580 */
    float height;                           /* +0x584 */
    unsigned char unmodeled_588[0x870 - 0x588];
} EnepcRecord;

extern EnepcRecord uwamonoRecords[];

/* A path point of a script array: integer coordinates in script units (x 100). */
typedef struct EnepcPathPoint {
    int x;
    int y;
    int z;
} EnepcPathPoint;

/* A route node of a script array: coordinates as above and the node id. */
typedef struct EnepcRouteNode {
    int x;
    int y;
    int z;
    short node;
    unsigned char unmodeled_0e[2];
} EnepcRouteNode;

/* One preset of D_0069D1D8, handed to enepcStoreParams in its argument order. */
typedef struct EnepcParamSet {
    float height;                           /* +0x00 */
    float extent;                           /* +0x04 */
    float floatParams[4];                   /* +0x08 */
    float angleDegrees;                     /* +0x18 */
    float floatParams48[2];                 /* +0x1C */
    unsigned char unmodeled_24[4];
    short shortParams[4];                   /* +0x28 */
} EnepcParamSet;

extern EnepcParamSet *D_0069D1D8;

int uwamonoFindIndexByShortKey(short key);
void *func_A9F298(int call, int handle);
void uwamonoAddRouteNode(int index, short node, float x, float y, float z);
void uwamonoCompactRoute(int index, short mode);
void enepcStoreParams(int index, float height, float extent,
                      float paramA0, float paramA1, float paramA2, float paramA3,
                      float angleDegrees, float paramB0, float paramB1,
                      short shortParam0, short shortParam1, short shortParam2, short shortParam3,
                      float actorParam14C, short motion, float actorParam170, float actorParam174,
                      int initState);

/* Vibration native, at least 4 arguments.  When func_A9F1C0 reports argument 1 as
   absent (nonzero), the fade is stopped (func_00208B28); otherwise argument 1 goes to
   func_00208B50 and arguments 2 to 4 start a vibrationStartFade. */
int runtime_001F4E20(int call)
{
    int value;
    int targetValue;
    int currentValue;
    int framesTotal;

    if (func_A9F0A8() < 4) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (func_A9F1C0(call, 1) != 0) {
        func_00208B28();
        return 1;
    }
    value = func_A9F0C0(call, 1);
    targetValue = func_A9F0C0(call, 2);
    currentValue = func_A9F0C0(call, 3);
    framesTotal = func_A9F0C0(call, 4);
    func_00208B50(value);
    vibrationStartFade(targetValue, currentValue, framesTotal);
    return 1;
}

extern unsigned char runtimeNativeTable[];
extern unsigned char runtimeModuleName[];
void func_A9CDB0(void *, void *);

/* Registers the "runtime" script module: runtimeModuleName holds its name string and
   runtimeNativeTable its table of name/function pointer pairs (setNumber, setLocater,
   setElevator, ...).  Called from scriptRegisterModules. */
void runtimeRegisterNatives(void) {
    func_A9CDB0(runtimeModuleName, runtimeNativeTable);
}

/* enepc setPath native, 7 arguments: argument 1 is the record key, 3 a preset of
   D_0069D1D8 (absent: none), 4 and 5 a path point array and its count, 6 and 7 a
   route node array and its count.  Argument 2 is read but not used.  A path in route
   mode 9 is mirrored back onto itself (mode 8). */
int enepc_001F4F28(int call)
{
    int key;
    int index;
    int paramSet;
    EnepcPathPoint *points;
    int pointCount;
    EnepcRouteNode *nodes;
    int nodeCount;
    EnepcState *state;
    short shortParam3;
    int i;
    int j;

    if (func_A9F0A8() < 7) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    if (index == -1) {
        return 1;
    }
    func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 3) == 0) {
        paramSet = func_A9F0C0(call, 3);
    } else {
        paramSet = -1;
    }
    if (func_A9F1C0(call, 4) == 0) {
        points = func_A9F298(call, func_A9F0C0(call, 4));
    } else {
        points = NULL;
    }
    pointCount = func_A9F0C0(call, 5);
    if (func_A9F1C0(call, 6) == 0) {
        nodes = func_A9F298(call, func_A9F0C0(call, 6));
    } else {
        nodes = NULL;
    }
    nodeCount = func_A9F0C0(call, 7);
    state = uwamonoRecords[index].state;
    if (points != NULL) {
        state->pointCount = pointCount;
        for (i = 0; i < state->pointCount; i++) {
            state->points[i].coords[0] = (float) points[i].x / 100.0f;
            state->points[i].coords[1] = (float) points[i].y / 100.0f;
            state->points[i].coords[2] = (float) points[i].z / 100.0f;
        }
        if (state->routeMode == 9) {
            for (i = 0; i < state->pointCount - 2; i++) {
                state->points[state->pointCount + i].coords[0] = state->points[state->pointCount - i - 2].coords[0];
                state->points[state->pointCount + i].coords[2] = state->points[state->pointCount - i - 2].coords[2];
            }
            state->pointCount = state->pointCount * 2 - 2;
            if (state->routeMode == 9) {
                state->routeMode = 8;
            }
        }
        for (j = 0; j < state->pointCount; j++) {
            for (i = 0; i < 4; i++) {
                state->pointLinks[j][i] = -1;
                state->pointLinkValues[j][i] = 0;
            }
        }
    }
    if (nodes != NULL) {
        state->routeNodeCount = 0;
        for (i = 0; i < nodeCount; i++) {
            uwamonoAddRouteNode(index, nodes[i].node, (float) nodes[i].x / 100.0f,
                                (float) nodes[i].y / 100.0f, (float) nodes[i].z / 100.0f);
        }
    }
    uwamonoCompactRoute(index, 0);
    if (paramSet != -1 && D_0069D1D8 != NULL) {
        /* The fourth short of the preset is read before the flag is cleared: the
           original loads it ahead of every other argument. */
        shortParam3 = D_0069D1D8[paramSet].shortParams[3];
        state->flags &= ~4;
        enepcStoreParams(index, D_0069D1D8[paramSet].height, D_0069D1D8[paramSet].extent,
                         D_0069D1D8[paramSet].floatParams[0], D_0069D1D8[paramSet].floatParams[1],
                         D_0069D1D8[paramSet].floatParams[2], D_0069D1D8[paramSet].floatParams[3],
                         D_0069D1D8[paramSet].angleDegrees, D_0069D1D8[paramSet].floatParams48[0], D_0069D1D8[paramSet].floatParams48[1],
                         D_0069D1D8[paramSet].shortParams[0], D_0069D1D8[paramSet].shortParams[1],
                         D_0069D1D8[paramSet].shortParams[2], shortParam3,
                         -1000.0f, -1, -1000.0f, -1000.0f, 1);
    }
    return 1;
}

int func_001EF650(short key);

/* enepc setParams native, at least 15 arguments (fewer: error 2, returns 0).
   Argument 1 is the record key; the others are optional: arguments 2 to 10 are
   the floats of enepcStoreParams in script units (x 100; arguments 6 and 7 also
   x 1000), 11 to 14 its shorts, 16, 18 and 19 its actor floats, 17 its motion; -1 or
   -1000.0 when absent (func_A9F1C0 nonzero).  Argument 15, when not -1, sets bit 0x4 of
   the state flags; argument 20 absent makes enepcStoreParams run uwamonoInitState. */
int enepc_001F5360(int call)
{
    int key;
    int index;
    float height;
    float extent;
    float paramA0;
    float paramA1;
    float paramA2;
    float paramA3;
    float angleDegrees;
    float paramB0;
    float paramB1;
    short shortParam0;
    short shortParam1;
    short shortParam2;
    short shortParam3;
    signed char lockFlag;
    float actorParam14C;
    short motion;
    float actorParam170;
    float actorParam174;
    int initState;
    EnepcRecord *record;
    EnepcState *state;

    if (func_A9F0A8() < 15) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = func_001EF650((short) key);
    if (index == -1 || uwamonoRecords[index].actor == NULL) {
        return 1;
    }
    if (func_A9F1C0(call, 2) == 0) {
        height = (float) func_A9F0C0(call, 2) / 100.0f;
    } else {
        height = -1.0f;
    }
    if (func_A9F1C0(call, 3) == 0) {
        extent = (float) func_A9F0C0(call, 3) / 100.0f;
    } else {
        extent = -1.0f;
    }
    if (func_A9F1C0(call, 4) == 0) {
        paramA0 = (float) func_A9F0C0(call, 4) / 100.0f;
    } else {
        paramA0 = -1.0f;
    }
    if (func_A9F1C0(call, 5) == 0) {
        paramA1 = (float) func_A9F0C0(call, 5) / 100.0f;
    } else {
        paramA1 = -1.0f;
    }
    if (func_A9F1C0(call, 6) == 0) {
        paramA2 = (float) func_A9F0C0(call, 6) / 100.0f / 1000.0f;
    } else {
        paramA2 = -1.0f;
    }
    if (func_A9F1C0(call, 7) == 0) {
        paramA3 = (float) func_A9F0C0(call, 7) / 100.0f / 1000.0f;
    } else {
        paramA3 = -1.0f;
    }
    if (func_A9F1C0(call, 8) == 0) {
        angleDegrees = (float) func_A9F0C0(call, 8) / 100.0f;
    } else {
        angleDegrees = -1.0f;
    }
    if (func_A9F1C0(call, 9) == 0) {
        paramB0 = (float) func_A9F0C0(call, 9) / 100.0f;
    } else {
        paramB0 = -1.0f;
    }
    if (func_A9F1C0(call, 10) == 0) {
        paramB1 = (float) func_A9F0C0(call, 10) / 100.0f;
    } else {
        paramB1 = -1.0f;
    }
    if (func_A9F1C0(call, 11) == 0) {
        shortParam0 = func_A9F0C0(call, 11);
    } else {
        shortParam0 = -1;
    }
    if (func_A9F1C0(call, 12) == 0) {
        shortParam1 = func_A9F0C0(call, 12);
    } else {
        shortParam1 = -1;
    }
    if (func_A9F1C0(call, 13) == 0) {
        shortParam2 = func_A9F0C0(call, 13);
    } else {
        shortParam2 = -1;
    }
    if (func_A9F1C0(call, 14) == 0) {
        shortParam3 = func_A9F0C0(call, 14);
    } else {
        shortParam3 = -1;
    }
    if (func_A9F1C0(call, 15) == 0) {
        lockFlag = func_A9F0C0(call, 15);
    } else {
        lockFlag = -1;
    }
    if (func_A9F1C0(call, 16) == 0) {
        actorParam14C = (float) func_A9F0C0(call, 16) / 100.0f;
    } else {
        actorParam14C = -1000.0f;
    }
    if (func_A9F1C0(call, 17) == 0) {
        motion = func_A9F0C0(call, 17);
    } else {
        motion = -1;
    }
    if (func_A9F1C0(call, 18) == 0) {
        actorParam170 = (float) func_A9F0C0(call, 18) / 100.0f;
    } else {
        actorParam170 = -1000.0f;
    }
    if (func_A9F1C0(call, 19) == 0) {
        actorParam174 = (float) func_A9F0C0(call, 19) / 100.0f;
    } else {
        actorParam174 = -1000.0f;
    }
    initState = func_A9F1C0(call, 20) != 0;
    record = &uwamonoRecords[index];
    state = record->state;
    if (lockFlag != -1) {
        state->flags |= 4;
    }
    enepcStoreParams(index, height, extent, paramA0, paramA1, paramA2, paramA3,
                     angleDegrees, paramB0, paramB1,
                     shortParam0, shortParam1, shortParam2, shortParam3,
                     actorParam14C, motion, actorParam170, actorParam174, initState);
    return 1;
}

void uwamonoInitState(int index);

/* A statement macro: the original keeps this tail call a jal plus epilogue; 2.96 does
   not turn a call inside a do { } while (0) statement into a sibling j. */
#define ENEPC_INIT_STATE(index) do { uwamonoInitState(index); } while (0)

/* Shared by the enepc setParams natives.  For uwamonoRecords[index] whose kind byte is
   4 or 1, every argument that is not its sentinel (-1 for the shorts and most floats,
   -1000.0 for the three actor floats) is stored in its field; the angle is given in
   degrees.  uwamonoInitState runs when initState is 1. */
void enepcStoreParams(int index, float height, float extent,
                      float paramA0, float paramA1, float paramA2, float paramA3,
                      float angleDegrees, float paramB0, float paramB1,
                      short shortParam0, short shortParam1, short shortParam2, short shortParam3,
                      float actorParam14C, short motion, float actorParam170, float actorParam174,
                      int initState)
{
    EnepcRecord *record = &uwamonoRecords[index];
    struct XenoActor *actor;
    EnepcState *state;

    if (record->kind == 4 || record->kind == 1) {
        actor = record->actor;
        state = record->state;
        if (height != -1.0f) {
            record->height = height;
        }
        if (extent != -1.0f) {
            actor->radius = extent;
            record->extent = extent;
        }
        if (paramA0 != -1.0f) {
            state->floatParams[0] = paramA0;
        }
        if (paramA1 != -1.0f) {
            state->floatParams[1] = paramA1;
        }
        if (paramA2 != -1.0f) {
            state->floatParams[2] = paramA2;
        }
        if (paramA3 != -1.0f) {
            state->floatParams[3] = paramA3;
        }
        if (angleDegrees != -1.0f) {
            state->angle = angleDegrees / 180.0f * 3.1415927f;
        }
        if (paramB0 != -1.0f) {
            state->floatParams48[0] = paramB0;
        }
        if (paramB1 != -1.0f) {
            state->floatParams48[1] = paramB1;
        }
        if (shortParam0 != -1) {
            state->shortParams[0] = shortParam0;
        }
        if (shortParam1 != -1) {
            state->shortParams[1] = shortParam1;
        }
        if (shortParam2 != -1) {
            state->shortParams[2] = shortParam2;
        }
        if (shortParam3 != -1) {
            state->shortParams[3] = shortParam3;
        }
        if (actorParam14C != -1000.0f) {
            actor->actorParam14C = actorParam14C;
        }
        if (motion != -1) {
            record->motion = motion;
        }
        if (actorParam170 != -1000.0f) {
            actor->actorParam170 = actorParam170;
        }
        if (actorParam174 != -1000.0f) {
            actor->actorParam174 = actorParam174;
        }
        if (initState == 1) {
            ENEPC_INIT_STATE(index);
        }
    }
}

void uwamonoSetMotion(int index, int motion, float speed);
int XenoActorGetSlotRange(struct XenoActor *actor, int slot, unsigned int entry, int *start, int *end);

/* enepc native, at least 6 arguments: argument 1 is the record key, argument 2 the
   motion (group * 100 + number), argument 3 the speed in percent (default 100.0),
   arguments 4 and 5 the play range (-1 or absent: keep the slot's bound).  Argument 6
   present clears the actor's loop bit 0x2, absent sets it.  The motion is only restarted
   when it differs from the actor's current one. */
int enepc_001F5B60(int call)
{
    int key;
    int index;
    int motion;
    float speed;
    int start;
    int end;
    int clearLoop;
    struct XenoActor *actor;
    int rangeStart;
    int rangeEnd;

    if (func_A9F0A8() < 6) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    if (index == -1) {
        return 1;
    }
    motion = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 3) == 0) {
        speed = (float) func_A9F0C0(call, 3) / 100.0f;
    } else {
        speed = 100.0f;
    }
    if (func_A9F1C0(call, 4) == 0) {
        start = func_A9F0C0(call, 4);
    } else {
        start = -1;
    }
    if (func_A9F1C0(call, 5) == 0) {
        end = func_A9F0C0(call, 5);
    } else {
        end = -1;
    }
    clearLoop = func_A9F1C0(call, 6) == 0;
    if (uwamonoRecords[index].kind != 4 && uwamonoRecords[index].kind != 1) {
        return 1;
    }
    actor = uwamonoRecords[index].actor;
    if (actor->modeGroup != motion / 100 || actor->mode != motion % 100) {
        uwamonoSetMotion(index, motion, speed);
        XenoActorSetChannelValue(actor, 4.0f);
        XenoActorGetSlotRange(actor, 0, motion, &rangeStart, &rangeEnd);
        if (start == -1 && end != -1) {
            XenoActorSetPlayRange(actor, 0, end);
        }
        if (start != -1 && end == -1) {
            XenoActorSetPlayRange(actor, start, rangeEnd);
            actor->playStart = start;
        }
        if (start != -1 && end != -1) {
            XenoActorSetPlayRange(actor, start, end);
            actor->playStart = start;
        }
    }
    if (clearLoop == 1) {
        actor->flags &= ~2;
    } else {
        actor->flags |= 2;
    }
    return 1;
}
