#include "common.h"
#include "shared.h"
#include "main/get.h"

/* Named values of the map object (uwamono) system in this file. */
enum UwamonoLimit {
    UWAMONO_RECORD_COUNT = 256,
    UWAMONO_RING_SIZE = 128,
    UWAMONO_SPLINE_STEPS = 32    /* GetBSplineLoop steps per route point */
};

enum UwamonoScanState {
    UWAMONO_SCAN_RECORD = 0,     /* a ready record overlaps the probed point */
    UWAMONO_SCAN_NO_GROUND = 64, /* the map found no ground under the point */
    UWAMONO_SCAN_GROUND = 99     /* the map found ground under the point */
};

/*
 * The mode of a slot (state->mode) and the action it is entered with are the
 * same value: uwamonoSetAction stores its argument in state->mode. The names
 * follow the handler each value dispatches to in uwamonoUpdateRecord.
 */
enum UwamonoMode {
    UWAMONO_MODE_INIT = 0,          /* uwamonoInitState, then the next action */
    UWAMONO_MODE_WANDER = 1,        /* uwamonoNextAction: random walk in the area */
    UWAMONO_MODE_WAIT = 2,          /* uwamonoTickWait */
    UWAMONO_MODE_ANCHOR = 3,        /* uwamonoOrbitAnchor */
    UWAMONO_MODE_AIM = 4,           /* uwamonoAimAtCurrent */
    UWAMONO_MODE_PURSUE = 5,        /* uwamonoPursueFocus */
    UWAMONO_MODE_COOLDOWN = 6,      /* uwamonoTickCooldown */
    UWAMONO_MODE_SCRIPT = 8,        /* uwamonoStepScript */
    UWAMONO_MODE_ALERT = 9,         /* uwamonoAdvanceTimer; entered with the cooldown and effect set up */
    UWAMONO_MODE_ROUTE = 10,        /* uwamonoStepRoute */
    UWAMONO_MODE_SPLINE = 11,       /* uwamonoFollowSpline */
    UWAMONO_MODE_START = 12,        /* entered by uwamonoInitState; moves on to a wander or a route */
    UWAMONO_MODE_SPLINE_START = 13  /* moves on to the spline mode */
};

void mapProbeInit(void *probe);
float mapMoveProbe(Vector4 *position, int option, void *probe);

/*
 * One 0x870-byte slot of uwamonoSlotTable. The actor body pointer at +0x00 and the
 * state block pointer at +0x04 are the only fields modeled here; the rest of
 * the slot is untouched.
 */
typedef struct UwamonoSlot UwamonoSlot;

typedef struct UwamonoActor UwamonoActor;

typedef struct UwamonoState UwamonoState;

typedef struct UwamonoRecord UwamonoRecord;

/* A record position as a vector or as two doublewords. */
typedef union UwamonoPosition {
    AlignedVector4 vector;
    u64 words[2];
} UwamonoPosition;

/* A route or pending row: four floats or their bit patterns. */
typedef union UwamonoRow {
    Vector4 v;
    float f[4];
    u32 w[4];
} UwamonoRow;

/* One 0x870-byte record of uwamonoRecords as this file reads it (tu051.h's UwamonoRecord is the shared view). */
struct UwamonoRecord {
    short id;                           /* +0x00 */
    unsigned char kind;                 /* +0x02: 1 and 4 take part in uwamonoScanTargets */
    unsigned char unmodeled_03;
    int key;                            /* +0x04 */
    unsigned char unmodeled_08[0x10 - 0x08];
    UwamonoPosition position;              /* +0x10: the point, also copied as two doublewords */
    unsigned char unmodeled_20[0x44 - 0x20];
    float angle;                        /* +0x44 */
    unsigned char unmodeled_48[0x330 - 0x48];
    UwamonoActor *actor;                /* +0x330: the same two pointers as the head of uwamonoSlotTable[i] */
    UwamonoState *state;                /* +0x334 */
    float form;                         /* +0x338 */
    unsigned char unmodeled_33c[0x546 - 0x33C];
    s16 target;                         /* +0x546 */
    unsigned char unmodeled_548[0x570 - 0x548];
    int flags;                          /* +0x570 */
    unsigned char unmodeled_574[0x580 - 0x574];
    AlignedVector4 extent;              /* +0x580: x is the form size (header type) */
    unsigned char unmodeled_590[0x870 - 0x590];
};

extern UwamonoRecord uwamonoRecords[];

/*
 * Union views (style debt): a field read or written through one of these has
 * alias set 0, which the original scheduling of some functions below shows.
 */
typedef union UwamonoVectorView {
    Vector4 vector;
} UwamonoVectorView;

typedef union UwamonoWordView {
    int word;
    float value;
} UwamonoWordView;

typedef union UwamonoHalfView {
    u16 half;
    float word;
} UwamonoHalfView;

typedef union UwamonoFlagsView {
    int flags;
    long long flags64;
} UwamonoFlagsView;

extern int uwamonoCurrentIndex;

/* The actor body of a slot (the XenoActor of the record; only the fields this file uses). */
struct UwamonoActor {
    unsigned char unmodeled_000[0x10];
    AlignedVector4 position;            /* +0x010 */
    unsigned char unmodeled_020[0x50 - 0x20];
    AlignedVector4 rotation;            /* +0x050: y is the facing angle */
    AlignedVector4 direction;           /* +0x060: y is the moving angle */
    unsigned char unmodeled_070[0xB0 - 0x70];
    float radius;                       /* +0x0B0 */
    unsigned char unmodeled_0b4[0xB8 - 0xB4];
    int flags;                          /* +0x0B8 */
    unsigned char unmodeled_0bc[0x5D4 - 0xBC];
    int mode;                           /* +0x5D4 */
    int modeSlot;                       /* +0x5D8 */
    unsigned char unmodeled_5dc[0x654 - 0x5DC];
    float motionSpeed;                  /* +0x654 */
    unsigned char unmodeled_658[0x740 - 0x658];
    unsigned char hitBuffer[8];         /* +0x740: the result block of mapMoveProbe */
    u16 hitFlags;                       /* +0x748 */
    unsigned char unmodeled_74a[0x74F - 0x74A];
    u8 probeEnabled;                    /* +0x74F */
    unsigned char unmodeled_750[0x758 - 0x750];
    int ground;                         /* +0x758 */
    unsigned char unmodeled_75c[0x768 - 0x75C];
    float probeRadius;                  /* +0x768 */
    unsigned char unmodeled_76c[0x798 - 0x76C];
    int kind;                       /* +0x798 */
};

/* The state block of a slot. The route, motion and script views share it. */
struct UwamonoState {
    unsigned char unmodeled_0000[0x0C];
    u8 mode;                            /* +0x0C */
    unsigned char unmodeled_000d;
    s16 focusIndex;                     /* +0x0E: the record the slot aims at */
    Vector4 target;                     /* +0x10: the route point the slot moves toward */
    Vector4 area;                       /* +0x20 */
    float areaSize;                     /* +0x30 */
    float wanderStep;                     /* +0x34 */
    float leashRange;                     /* +0x38 */
    float moveSpeed;                    /* +0x3C */
    float pursuitSpeed;                     /* +0x40 */
    float fanWidth;                     /* +0x44 */
    float fanLength;                    /* +0x48 */
    float noticeRange;                      /* +0x4C */
    u16 wanderTime;                        /* +0x50 */
    s16 pickCountA;                     /* +0x52: the upper bound of the first random pick */
    u16 waitTime;                        /* +0x54 */
    s16 pickCountB;                     /* +0x56: the upper bound of the second random pick */
    s16 step;                           /* +0x58 */
    u16 timer;                          /* +0x5A: ticks since the current action started */
    s16 timerLimit;                     /* +0x5C */
    signed char alert;                  /* +0x5E */
    u8 action;                          /* +0x5F */
    u16 cooldown;                       /* +0x60 */
    unsigned char pickRow[2][8];        /* +0x62 */
    s16 pickIndexA;                       /* +0x72 */
    s16 pickIndexB;                       /* +0x74 */
    u8 reactionKind;                    /* +0x76 */
    u8 flags;                           /* +0x77 */
    s16 field78;                        /* +0x78 */
    u16 field7A;                        /* +0x7A */
    union {
        int fanFlags;                   /* +0x7C */
        float fanValue;
    };
    s16 groundFrame;                    /* +0x80 */
    unsigned char unmodeled_0082[0x90 - 0x82];
    union {
        Vector4 route[256];             /* +0x90 */
        UwamonoRow routeRows[256];
    };
    s16 routeCount;                     /* +0x1090 */
    unsigned char unmodeled_1092[0x10A0 - 0x1092];
    UwamonoRow pending[256];            /* +0x10A0 */
    union {
        s16 motionSpeed[1024];          /* +0x20A0 */
        s16 actionTable[256][4];        /* the action for each route index and script cursor */
    };
    u16 motionTime[1024];               /* +0x28A0 */
    s16 routeIndex;                     /* +0x30A0 */
    s16 routePrev;                      /* +0x30A2 */
    s16 motionCol;                      /* +0x30A4 */
    s16 pendingCount;                   /* +0x30A6 */
    unsigned char unmodeled_30a8[0x30B0 - 0x30A8];
    UwamonoRow points[64];              /* +0x30B0 */
    s16 links[64][4];                   /* +0x34B0 */
    s16 linkCount[64];                  /* +0x36B0 */
    unsigned char unmodeled_3730[0x37B0 - 0x3730];
    u16 pointCount;                     /* +0x37B0 */
    unsigned char unmodeled_37b2[0x37C0 - 0x37B2];
    Vector4 ring[128];                  /* +0x37C0 */
    s16 ringHead;                       /* +0x3FC0 */
    s16 ringTail;                       /* +0x3FC2 */
    unsigned char unmodeled_3fc4[0x3FE0 - 0x3FC4];
    Vector4 probe;                      /* +0x3FE0 */
    int blockFlags;                     /* +0x3FF0 */
    unsigned char unmodeled_3ff4[0x4000 - 0x3FF4];
    DoublewordAlignedVector4 homePoint; /* +0x4000 */
    int voiceId[2];                     /* +0x4010 */
    unsigned char unmodeled_4018[0x401E - 0x4018];
    s16 noticeCooldown;                      /* +0x401E */
    int scanState;                      /* +0x4020 */
    int targetRecord;                   /* +0x4024 */
    s16 priority;                       /* +0x4028 */
    unsigned char unmodeled_402a[0x4040 - 0x402A];
    u16 field4040;                      /* +0x4040 */
    u16 candidates[8];                  /* +0x4042 */
    unsigned char unmodeled_4052[0x4064 - 0x4052];
    s16 field4064;                      /* +0x4064 */
    unsigned char unmodeled_4066[0x4068 - 0x4066];
    void *handle;                       /* +0x4068: the effect record */
    unsigned char unmodeled_406c[0x4078 - 0x406C];
    int motionValues[16];               /* +0x4078 */
};

struct UwamonoSlot {
    UwamonoActor *actor; /* +0x00 */
    UwamonoState *state; /* +0x04 */
    unsigned char unmodeled_08[0x870 - 0x08];
};

void uwamonoSetAction(int slot_index, signed char action);
void uwamonoRebuildTrail(int index);
void sefReleaseRecordHandle(void *record);
float Get_Angle(const Vector4 *first, const Vector4 *second);
float Get_Distance(const Vector4 *first, const Vector4 *second);
int uwamonoMoveAlongHeading(int slotIndex, Vector4 *target, float speed);
/*
 * Starts a motion on the actor of one slot. A motion below 16 indexes the
 * state's motion table; the value splits into a mode slot (value / 100) and a
 * mode (value % 100). The actor is re-set to the channel value 4.0 and given
 * the mode unless it already holds that slot and mode.
 */
void uwamonoSetMotion(int slotIndex, int motion, float speed);
/*
 * Copies the point into the record's position when it lies within 0.1 of the
 * record and the mode is 1. Returns 0 when the point is out of reach, else 1.
 */
int uwamonoCopyPointIfNear(int index, const Vector4 *point, signed char mode);
extern UwamonoSlot uwamonoSlotTable[];

/*
 * Sets up the state of one slot. The record key decides whether the slot's motion
 * table keeps flag 1, two random byte sets are drawn from the pick counts, the
 * probe is reset and moved to the record's probe radius, and the slot enters
 * action 12.
 */
void uwamonoInitState(int slotIndex)
{
    UwamonoRecord *record;
    UwamonoState *state;
    UwamonoActor *actor;
    void *probe;
    s16 i;

    record = &uwamonoRecords[slotIndex];
    actor = uwamonoRecords[slotIndex].actor;
    state = uwamonoRecords[slotIndex].state;

    ((UwamonoVectorView *) &actor->direction)->vector.y = actor->rotation.y;
    if ((u32) (record->key - 0x18000) < 0x101F)
    {
        state->blockFlags |= 1;
    }
    else
    {
        state->blockFlags &= ~1;
    }
    i = 0;
    for (; i < 8; i++)
    {
        state->pickRow[1][i] = 0;
        state->pickRow[0][i] = 0;
    }
    state->pickIndexA = 0;
    state->pickIndexB = 0;
    if (state->pickCountA == 1)
    {
        state->pickCountA = 2;
    }
    if (state->pickCountB == 1)
    {
        state->pickCountB = 2;
    }
    if (state->pickCountA >= 8)
    {
        state->pickCountA = 7;
    }
    if (state->pickCountB >= 8)
    {
        state->pickCountB = 7;
    }
    if (state->pickCountA != 0)
    {
        state->pickRow[0][Get_Rnd(1, state->pickCountA) % 8] = 1;
    }
    if (state->pickCountB != 0)
    {
        state->pickRow[1][Get_Rnd(1, state->pickCountB) % 8] = 1;
    }
    probe = actor->hitBuffer;
    uwamonoSetAction(slotIndex, 12);
    state->timer = 0;
    state->timerLimit = 0;
    mapProbeInit(probe);
    ((UwamonoHalfView *) &actor->hitFlags)->half = 2;
    actor->probeEnabled = 2;
    {
        float radius = record->extent.x;

        actor->probeRadius = radius;
    }
    mapMoveProbe(&actor->position, 0, actor->hitBuffer);
    state->routeIndex = 0;
    state->cooldown = 0;
}

/*
 * Advances the action timer of one slot. When the timer reaches its limit the
 * actor drops flag 0x800000 and uwamonoSetAction enters action 6 if state flag 1
 * is set, otherwise action 5.
 */
void uwamonoAdvanceTimer(int slotIndex)
{
    UwamonoSlot *slot;
    UwamonoState *state;
    UwamonoActor *actor;
    u16 timer;
    int flagBit;

    slot = &uwamonoSlotTable[slotIndex];
    state = slot->state;
    actor = slot->actor;
    timer = state->timer;
    state->timer = (u16) (timer + 1);
    if ((s16) timer < state->timerLimit)
    {
        return;
    }
    actor->flags &= 0xFF7FFFFF;
    flagBit = 1;
    if (state->flags & flagBit)
    {
        uwamonoSetAction(slotIndex, 6);
        return;
    }
    uwamonoSetAction(slotIndex, 5);
}

/*
 * Counts down the cooldown of one slot. When the count is no longer positive
 * the slot leaves the cooldown: its state flag 1 and actor flag 0x01000000 are
 * cleared, the effect record is released, and the slot returns to action 2.
 */
int uwamonoTickCooldown(int slotIndex)
{
    UwamonoSlot *slot;
    UwamonoState *state;
    UwamonoActor *actor;
    int remaining;

    slot = &uwamonoSlotTable[slotIndex];
    state = slot->state;
    actor = slot->actor;
    remaining = state->cooldown - 1;
    state->cooldown = (u16) remaining;
    if ((s16) remaining <= 0)
    {
        state->cooldown = 0;
        state->flags &= 0xFE;
        actor->flags &= 0xFEFFFFFF;
        sefReleaseRecordHandle(state->handle);
        state->handle = 0;
        uwamonoRebuildTrail(slotIndex);
        uwamonoSetAction(slotIndex, 2);
    }
}

/*
 * Advances the action timer of one slot and turns its actor toward the current
 * record. When the timer reaches its limit the slot enters action 5. The actor's
 * moving angle is then set from its position toward uwamonoRecords[uwamonoCurrentIndex].
 */
void uwamonoAimAtCurrent(int slotIndex)
{
    UwamonoState *state;
    UwamonoActor *actor;
    u16 timer;

    actor = uwamonoRecords[slotIndex].actor;
    state = uwamonoRecords[slotIndex].state;
    timer = state->timer;
    state->timer = (u16) (timer + 1);
    if ((s16) timer >= state->timerLimit)
    {
        uwamonoSetAction(slotIndex, 5);
    }
    actor->direction.y = Get_Angle(&actor->position, &uwamonoRecords[uwamonoCurrentIndex].position.vector);
}

int uwamonoScanTargets(int slotIndex, float range);
void func_00202C78(int slotIndex);
/* Declared without a prototype: uwamonoPursueFocus passes the slot index, the definition takes none. */
void uwamonoHaltPursuit();
void uwamonoClampToStopRange(int slotIndex);
void XenoActorSetChannelValue(void *actor, float value);
void XenoActorEnableChannel(void *actor);
void XenoActorCheckMode(void *actor, int mode);
void vectorCopyPoint(Vector4 *destination, Vector4 *source, float angle);
int uwamonoCrossPoint(Vector4 *from, Vector4 *to, Vector4 *out, float radius);
int unduCheckSweep(int slotIndex, const Vector4 *from, const Vector4 *to, int mask, s16 bits, Vector4 *hit, int extra);
int unduCheckMoveSquare(Vector4 *from, Vector4 *to, Vector4 *out, float radius, int mask);
void xglFontDebugPrintf(int x, int y, const char *format, ...);
float sinf(float x);
float cosf(float x);
float Get_Decimal_Surplus_for_Radius(float value);
void uwamonoCompactRoute(int index, short step);
extern const char D_00689DC8[];
extern const char D_00689DD8[];
extern float uwamonoPi;
extern float D_0069B0E8;
extern s16 D_002B60C0[];
extern s16 uwamonoLadderState;
extern float D_0069B0D4;
extern float D_0069B0D8;
extern float D_0069B0DC;
extern float D_0069B0E0;
extern const char D_00689D90[];
extern const char D_00689DA8[];
extern const char D_0069D448[];
extern const char D_0069D438[];
extern const char D_0069D440[];
extern const char D_00689DB8[];

/*
 * Moves one slot toward its focus record along the ring of move points. While the
 * slot has a focus, the current move point is taken from the ring head and the
 * sweep and move tests choose the next point; without a focus the slot follows
 * the nearest route point and turns to face the focus record.
 */
void uwamonoPursueFocus(int slotIndex)
{
    UwamonoRecord *record;
    Vector4 *ringCursor;
    UwamonoActor *actor;
    UwamonoState *state;
    DoublewordAlignedVector4 startPosition;
    Vector4 hit;
    Vector4 crossPoint;
    struct { int r, g, b, a; } color;
    DoublewordAlignedVector4 squarePoint;
    DoublewordAlignedVector4 destination;
    float speed;
    float dist;
    float limit;
    int crossResult;
    int moveResult;
    const char *fmt;

    record = &uwamonoRecords[slotIndex];
    actor = uwamonoRecords[slotIndex].actor;
    state = uwamonoRecords[slotIndex].state;

    if (D_002B60C0[0] > 0)
    {
        uwamonoHaltPursuit(slotIndex);
        return;
    }
    func_00202C78(slotIndex);

    if (record->target != -1)
    {
        state->homePoint = state->ring[state->ringHead];
        destination = state->homePoint;
        if (uwamonoScanTargets(slotIndex, state->pursuitSpeed + 0.1f) == 0)
        {
            XenoActorSetChannelValue(actor, 4.0f);
            XenoActorEnableChannel(actor);
            actor->motionSpeed = 1.0f;
            XenoActorCheckMode(actor, 0);
            return;
        }
    }
    else
    {
        destination = uwamonoRecords[state->focusIndex].position.vector;
    }

    dist = Get_Distance(&actor->position, (&state->homePoint));
    if (dist >= 1.0f)
    {
        speed = state->pursuitSpeed * 3.0f;
        if (record->target != -1)
        {
            speed = speed * 8.0f;
        }
    }
    else
    {
        speed = state->pursuitSpeed;
        if (record->target != -1)
        {
            speed = speed * 16.0f;
        }
    }

    startPosition = actor->position;
    unduCheckSweep(slotIndex, &actor->position, &destination, state->fanFlags, state->groundFrame, &hit, 0);
    crossResult = uwamonoCrossPoint(&actor->position, &destination, &crossPoint, actor->radius);
    if (__builtin_fabsf(hit.x - destination.x) < 0.01f && __builtin_fabsf(hit.z - destination.z) < 0.01f && crossResult == 0)
    {
        state->homePoint = destination;
        fmt = D_00689D90;
        xglFontDebugPrintf(8, 100, fmt, (double) state->homePoint.x, (double) state->homePoint.y, (double) state->homePoint.z);
    }
    else if (crossResult == 1)
    {
        if (unduCheckMoveSquare(&destination, &actor->position, &squarePoint, actor->radius, state->fanFlags) == 1)
        {
            state->homePoint = squarePoint;
            xglFontDebugPrintf(8, 112, D_0069D438);
        }
    }
    else
    {
        if (unduCheckMoveSquare(&destination, &actor->position, &squarePoint, actor->radius, state->fanFlags) == 1)
        {
            state->homePoint = squarePoint;
            xglFontDebugPrintf(8, 112, D_00689DA8, state->focusIndex);
        }
    }

    state->target = state->homePoint;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    color.a = 128;
    moveResult = uwamonoMoveAlongHeading(slotIndex, (&state->homePoint), speed);
    if (moveResult == 0 && (record->flags & 0x8000) != 0)
    {
        uwamonoSetAction(slotIndex, 2);
        uwamonoRebuildTrail(slotIndex);
    }

    if (state->alert == 96)
    {
        if (unduCheckMoveSquare(&crossPoint, &actor->position, &squarePoint, actor->radius, state->fanFlags) == 1)
        {
            state->homePoint = squarePoint;
            xglFontDebugPrintf(8, 124, D_0069D438);
        }
        else
        {
            state->homePoint = crossPoint;
            xglFontDebugPrintf(8, 124, D_0069D440);
        }
    }


    if (record->target == -1)
    {
        if (state->leashRange <= Get_Distance(&actor->position, &uwamonoRecords[state->focusIndex].position.vector)
            || state->areaSize - actor->radius <= Get_Distance(&state->area, &actor->position)
            || state->alert == 88 || state->alert == 64 || uwamonoLadderState != 0
            || (Get_Distance(&actor->position, (&state->homePoint)) < 0.01f && state->alert != 96 && state->alert != 99))
        {
            uwamonoClampToStopRange(slotIndex);
            state->step = 0;
            vectorCopyPoint(state->route, &actor->position, ((UwamonoVectorView *) &actor->direction)->vector.y);
            state->routeCount = 1;
            uwamonoSetAction(slotIndex, 2);
            uwamonoRebuildTrail(slotIndex);
            return;
        }
        if (Get_Distance(&actor->position, (&state->homePoint)) < 0.05f)
        {
            if (unduCheckMoveSquare(&state->probe, &actor->position, &squarePoint, actor->radius, state->fanFlags) == 1)
            {
                state->homePoint = squarePoint;
                xglFontDebugPrintf(8, 112, D_0069D448);
                uwamonoMoveAlongHeading(slotIndex, (&state->homePoint), speed);
            }
            ((UwamonoVectorView *) &actor->direction)->vector.y = Get_Angle(&actor->position, &uwamonoRecords[state->focusIndex].position.vector);
        }
    }
    else
    {
        xglFontDebugPrintf(8, 90, D_00689DB8, (double) Get_Distance(&actor->position, (&state->homePoint)));
        limit = uwamonoRecords[record->target].extent.x * 0.0625f;
        if (limit <= Get_Distance(&actor->position, (&state->homePoint)))
        {
            if ((state->ringTail + 1) % UWAMONO_RING_SIZE != state->ringHead)
            {
                state->ring[state->ringTail] = uwamonoRecords[record->target].position.vector;
                state->ringTail = (state->ringTail + 1) % UWAMONO_RING_SIZE;
            }
        }
        while (Get_Distance(&actor->position, ringCursor = &state->homePoint) <= speed)
        {
            state->ringHead = (state->ringHead + 1) % UWAMONO_RING_SIZE;
            *ringCursor = state->ring[state->ringHead];
            if ((state->ringHead + 1) % UWAMONO_RING_SIZE == state->ringTail)
            {
                break;
            }
        }
    }
}

/* A view of the pick block of UwamonoState, which starts at +0x02. */
typedef struct UwamonoPickBlock {
    unsigned char unmodeled_00[0x60];
    signed char pickRow[2][8];   /* +0x62 in the state */
    s16 pickIndex[2];            /* +0x72 in the state */
} UwamonoPickBlock;

void uwamonoOrbitAnchor(int slotIndex)
{
    UwamonoSlot *slot;
    UwamonoState *state;
    UwamonoActor *actor;
    UwamonoPickBlock *pickView;
    s16 pickKind;
    int pickCount;
    s16 pickRowIndex;
    int offset;
    float angle;
    float dist;

    slot = &uwamonoSlotTable[slotIndex];
    state = slot->state;
    actor = slot->actor;
    pickCount = 0;
    state->timer++;
    if ((s16) state->timer == 1)
    {
        if (!(actor->flags & 0x80000000))
        {
            for (pickKind = 0; pickKind < 2; pickKind++)
            {
                if (pickKind == 0 && state->pickCountA == 0)
                {
                    continue;
                }
                if (pickKind == 1 && state->pickCountB == 0)
                {
                    continue;
                }
                switch (pickKind)
                {
                case UWAMONO_MODE_INIT:
                    pickCount = state->pickCountA;
                    break;
                case UWAMONO_MODE_WANDER:
                    pickCount = state->pickCountB;
                    break;
                }
                offset = pickKind * 2 + 112;
                pickView = (UwamonoPickBlock *) ((u8 *) state + 2);
                pickView->pickIndex[pickKind] = (s16) ((pickView->pickIndex[pickKind] + 1) % pickCount);
                if (pickView->pickIndex[pickKind] == 0)
                {
                    if (pickCount >= 8)
                    {
                        pickCount = 8;
                    }
                    for (pickRowIndex = 0; pickRowIndex < pickCount; pickRowIndex++)
                    {
                        pickView->pickRow[pickKind][pickRowIndex] = 0;
                    }
                    pickView->pickRow[pickKind][Get_Rnd(1, pickCount - 1)] = 1;
                }
                if (pickView->pickRow[pickKind][pickView->pickIndex[pickKind]] == 1)
                {
                    switch (pickKind)
                    {
                    case UWAMONO_MODE_INIT:
                        uwamonoSetAction(slotIndex, 1);
                        break;
                    case UWAMONO_MODE_WANDER:
                        uwamonoSetAction(slotIndex, 2);
                        break;
                    }
                    pickView->pickRow[0][pickView->pickIndex[pickKind]] = pickView->pickRow[1][pickView->pickIndex[pickKind]] = 0;
                    return;
                }
            }
        }

        xglFontDebugPrintf(8, 128, D_00689DC8);
        if (state->routeIndex == 0)
        {
            uwamonoCompactRoute(slotIndex, 0);
        }
        if (state->action == UWAMONO_MODE_WANDER && !(actor->flags & 0x80000000))
        {
            xglFontDebugPrintf(8, 136, D_00689DD8);
            offset = Get_Rnd(0, 179);
            angle = Get_Decimal_Surplus_for_Radius((actor->direction.y - D_0069B0E8) + 2.0f * (((float) offset / 360.0f) * uwamonoPi) + uwamonoPi);
            state->target.x = sinf(angle) * (state->areaSize - actor->radius) + state->area.x;
            state->target.z = cosf(angle) * (state->areaSize - actor->radius) + state->area.z;
            angle = Get_Angle(&actor->position, &state->target);
            dist = Get_Distance(&actor->position, &state->target);
            if (state->wanderStep < dist)
            {
                dist = state->wanderStep;
            }
            state->target.x = actor->position.x + sinf(angle) * dist;
            state->target.z = actor->position.z + cosf(angle) * dist;
            actor->direction.y = angle;
        }
        else
        {
            if (state->routeCount != 0)
            {
                state->routeIndex = (s16) ((state->routeIndex + 1) % state->routeCount);
                if (actor->flags < 0 && state->routeIndex == 0)
                {
                    actor->flags &= 0x7FFFFFFF;
                    uwamonoSetAction(slotIndex, 1);
                }
            }
            state->target = state->route[state->routeIndex];
        }
    }

    if (uwamonoMoveAlongHeading(slotIndex, &state->target, state->moveSpeed) == 0)
    {
        if (state->action == UWAMONO_MODE_SCRIPT)
        {
            uwamonoSetMotion(slotIndex, 0, 1.0f);
        }
        else
        {
            state->timer = 0;
        }
    }
    if (uwamonoCopyPointIfNear(slotIndex, &state->target, 0) == 1)
    {
        state->timer = 0;
    }
}

/*
 * Steps one slot along its route. On timer tick 1 the slot takes the route point
 * before, at or after its current index, whichever of the three is nearest to the
 * current record; it reaches that point when the timer is 1. Otherwise it keeps
 * moving toward its target and steps back when the move is complete.
 */
void uwamonoStepRoute(int slotIndex)
{
    UwamonoState *state;
    Vector4 neighbours[3];
    float neighbourDistance[3];
    float best;
    int bestIndex;
    s16 i;
    s16 routeLength;
    s16 currentIndex;

    best = 0.0f;
    bestIndex = -1;
    state = uwamonoRecords[slotIndex].state;
    state->timer++;
    if ((s16) state->timer == 1)
    {
        routeLength = state->routeCount;
        currentIndex = state->routeIndex;
        neighbours[0] = state->route[(currentIndex + routeLength - 1) % routeLength];
        neighbours[1] = state->route[currentIndex];
        neighbours[2] = state->route[(currentIndex + 1) % routeLength];
        for (i = 0; i < 3; i++)
        {
            neighbourDistance[i] = Get_Distance(&((UwamonoActor *) uwamonoRecords[uwamonoCurrentIndex].actor)->position, &neighbours[i]);
        }
        for (i = 0; i < 3; i++)
        {
            if (best <= neighbourDistance[i])
            {
                best = neighbourDistance[i];
                bestIndex = i;
            }
        }
        state->routePrev = state->routeIndex;
        state->routeIndex = (state->routeIndex + state->routeCount + bestIndex - 1) % state->routeCount;
        state->target = state->route[state->routeIndex];
        if (bestIndex == 1)
        {
            state->timer = 0;
            return;
        }
    }
    if (uwamonoMoveAlongHeading(slotIndex, &state->target, state->moveSpeed) == 0)
    {
        uwamonoSetMotion(slotIndex, 0, 1.0f);
        state->timer = 1;
        state->routeIndex = state->routePrev;
        state->target = state->route[state->routeIndex];
    }
    if (uwamonoCopyPointIfNear(slotIndex, &state->target, 1) == 1)
    {
        state->timer = 0;
    }
}

void GetBSplineLoop(s16 index, s16 count, int stride, void *points, Vector4 *out);
extern s16 D_0069D44E;

void uwamonoFollowSpline(int slotIndex)
{
    UwamonoSlot *slot = &uwamonoSlotTable[slotIndex];
    UwamonoState *state = slot->state;
    UwamonoActor *actor = slot->actor;
    Vector4 splinePoint;
    float probeDistance[3];
    int splineSteps;
    int motion;
    s16 nearest;
    s16 j;

    if (state->routeCount > 0)
    {
        for (j = 0; j < 3; j++)
        {
            s16 loopCount = state->routeCount;
            splineSteps = loopCount * UWAMONO_SPLINE_STEPS;
            GetBSplineLoop((s16) ((state->step + splineSteps + j - 1) % splineSteps), loopCount, UWAMONO_SPLINE_STEPS, state->route, &splinePoint);
            probeDistance[j] = Get_Distance(&state->probe, &splinePoint);
        }
        if (probeDistance[0] < probeDistance[1])
        {
            nearest = probeDistance[0] < probeDistance[2] ? 0 : 2;
        }
        else
        {
            nearest = probeDistance[1] < probeDistance[2] ? 1 : 2;
        }
        state->step = (s16) ((state->step + state->routeCount * UWAMONO_SPLINE_STEPS + nearest - 1) % (state->routeCount * UWAMONO_SPLINE_STEPS));
        GetBSplineLoop(state->step, state->routeCount, UWAMONO_SPLINE_STEPS, state->route, &splinePoint);
        if (nearest == 1)
        {
            actor->direction.y = Get_Angle(&actor->position, &state->probe);
            D_0069D44E = D_0069D44E - 1;
        }
        else
        {
            D_0069D44E = 16;
            actor->direction.y = Get_Angle(&actor->position, &splinePoint);
        }
        if (!(0.012f <= Get_Distance(&actor->position, &splinePoint)) && D_0069D44E <= 0)
        {
            motion = 1;
        }
        else
        {
            motion = 2;
        }
        uwamonoSetMotion(slotIndex, motion, 1.0f);
        actor->position.x = splinePoint.x;
        actor->position.z = splinePoint.z;
    }
}





unsigned short xglSRand(void);
void XenoActorSetModeInSlot(void *, int, int);


void uwamonoNextAction(int slotIndex)
{
    UwamonoState *state = uwamonoRecords[slotIndex].state;
    UwamonoActor *actor = uwamonoRecords[slotIndex].actor;
    u16 timer;

    if (state->action == UWAMONO_MODE_PURSUE || state->action == UWAMONO_MODE_COOLDOWN)
    {
        if (actor->flags & 0x400000)
        {
            actor->direction.y = Get_Angle(&actor->position, &uwamonoRecords[state->focusIndex].position.vector);
        }
    }
    timer = state->timer;
    state->timer = (u16) (timer + 1);
    if (state->timerLimit - 1 > (s16) timer)
    {
        return;
    }
    switch (state->action)
    {
    case UWAMONO_MODE_PURSUE:
    case UWAMONO_MODE_COOLDOWN:
        uwamonoSetAction(slotIndex, 1);
        return;
    case UWAMONO_MODE_WANDER:
        uwamonoSetAction(slotIndex, 3);
        state->step = 0;
        state->routeIndex = 0;
        state->timer = 0;
        return;
    case UWAMONO_MODE_SCRIPT:
        uwamonoSetAction(slotIndex, 3);
        state->step = 0;
        state->timer = 0;
        return;
    case UWAMONO_MODE_START:
        uwamonoSetAction(slotIndex, 10);
        return;
    case UWAMONO_MODE_SPLINE_START:
        uwamonoSetAction(slotIndex, 11);
        return;
    default:
    {
        int roll = xglSRand() & 1;

        switch (roll)
        {
        case UWAMONO_MODE_INIT:
            uwamonoSetAction(slotIndex, 3);
            return;
        case UWAMONO_MODE_WANDER:
            uwamonoSetAction(slotIndex, 2);
            return;
        }
        return;
    }
    }
}

/*
 * Ticks the timer of one slot. When the timer reaches its limit the action is
 * set to 3. Kind 8 also clears the step; any other kind clears the pending
 * count, and an actionFlags bit 0x10 advances the step by 2 first.
 */
void uwamonoTickWait(int slotIndex)
{
    UwamonoSlot *slot = &uwamonoSlotTable[slotIndex];
    UwamonoState *state = slot->state;
    u16 timer = state->timer;

    state->timer = (u16) (timer + 1);
    if (state->timerLimit - 1 > (s16) timer)
    {
        return;
    }
    if (state->action == UWAMONO_MODE_SCRIPT)
    {
        uwamonoSetAction(slotIndex, 3);
        (*(u16 *) &state->step) = 0;
        state->timer = 0;
        return;
    }
    uwamonoSetAction(slotIndex, 3);
    if (state->blockFlags & 0x10)
    {
        do
        {
            (*(u16 *) &state->step) += 2;
        } while (0);
        state->timer = 0;
    }
    else
    {
        state->timer = 0;
    }
    (*(u16 *) &state->routeIndex) = 0;
}

/*
 * Ticks the timer of one slot. At its limit the script cursor advances and the
 * action of actionTable for the pending count and the cursor is entered: -1 sets
 * action 3 and runs the expiry tail, any other value sets action 8.
 */
void uwamonoStepScript(int slotIndex)
{
    UwamonoSlot *slot = &uwamonoSlotTable[slotIndex];
    UwamonoState *state = slot->state;
    u16 timer = state->timer;
    s16 scriptAction;

    state->timer = (u16) (timer + 1);
    if (state->timerLimit - 1 > (s16) timer)
    {
        return;
    }
    (*(u16 *) &state->motionCol) = (u16) ((*(u16 *) &state->motionCol) + 1);
    scriptAction = state->actionTable[state->routeIndex][(s16) (*(u16 *) &state->motionCol)];
    if (scriptAction != -1)
    {
        uwamonoSetAction(slotIndex, 8);
        return;
    }
    uwamonoSetAction(slotIndex, 3);
    if (state->blockFlags & 0x10)
    {
        do
        {
            (*(u16 *) &state->step) += 2;
        } while (0);
        state->timer = 0;
    }
    else
    {
        state->timer = 0;
    }
    state->routeIndex = 0;
}



typedef struct UwamonoGameWork {
    unsigned char unmodeled_000[0x06];
    u16 field06;                        /* +0x06 */
    int control;                        /* +0x08 */
    unsigned char unmodeled_00c[0x10 - 0x0C];
    int flags;                          /* +0x10 */
    unsigned char unmodeled_014[0x1C - 0x14];
    int field1C;                        /* +0x1C */
    unsigned char unmodeled_020[0x31C - 0x20];
    s16 encounterTarget;                       /* +0x31C */
    unsigned char unmodeled_31e[2];
    s16 encounterReaction;                       /* +0x320 */
    unsigned char unmodeled_322[2];
    int encounterFlags;                       /* +0x324 */
    unsigned char unmodeled_328[0x32C - 0x328];
    int encounterRecord;                       /* +0x32C */
    s16 blocked;                        /* +0x330 */
    unsigned char unmodeled_332[0x344 - 0x332];
    void *releaseHandle;                /* +0x344 */
    unsigned char unmodeled_348[0x1B20 - 0x348];
    signed char field1B20;              /* +0x1B20 */
} UwamonoGameWork;

extern UwamonoGameWork gameWork;
extern signed char pxyTickScaleDouble;
extern unsigned short D_0063EDAA[];
extern float D_0069B0F0;
extern float D_0069B0F4;
extern float D_0069B0F8;
extern float D_0069B0FC;
extern char D_0069D450[];
void uwamonoDrawPointPath(int index);
void uwamonoDrawStateLinks(int index);
void DispPillar(Vector4 *position, float extent, float scale, int mode);
void uwamonoDrawFanEdge(Vector4 *position, float angle, float range, int mode);
/* * Sets up the state of one slot. The record key decides whether the slot's motion * table keeps flag 1, two random byte sets are drawn from the pick counts, the * probe is reset and moved to the record's probe radius, and the slot enters * action 12. */ void uwamonoInitState(int slotIndex);
void uwamonoNextAction(int slotIndex);
/* * Ticks the timer of one slot. When the timer reaches its limit the action is * set to 3. Kind 8 also clears the step; any other kind clears the pending * count, and an actionFlags bit 0x10 advances the step by 2 first. */ void uwamonoTickWait(int slotIndex);
void uwamonoOrbitAnchor(int slotIndex);
/* * Advances the action timer of one slot and turns its actor toward the current * record. When the timer reaches its limit the slot enters action 5. The actor's * moving angle is then set from its position toward uwamonoRecords[uwamonoCurrentIndex]. */ void uwamonoAimAtCurrent(int slotIndex);
/* * Moves one slot toward its focus record along the ring of move points. While the * slot has a focus, the current move point is taken from the ring head and the * sweep and move tests choose the next point; without a focus the slot follows * the nearest route point and turns to face the focus record. */ void uwamonoPursueFocus(int slotIndex);
/* * Counts down the cooldown of one slot. When the count is no longer positive * the slot leaves the cooldown: its state flag 1 and actor flag 0x01000000 are * cleared, the effect record is released, and the slot returns to action 2. */ int uwamonoTickCooldown(int slotIndex);
/* * Ticks the timer of one slot. At its limit the script cursor advances and the * action of actionTable for the pending count and the cursor is entered: -1 sets * action 3 and runs the expiry tail, any other value sets action 8. */ void uwamonoStepScript(int slotIndex);
/* * Steps one slot along its route. On timer tick 1 the slot takes the route point * before, at or after its current index, whichever of the three is nearest to the * current record; it reaches that point when the timer is 1. Otherwise it keeps * moving toward its target and steps back when the move is complete. */ void uwamonoStepRoute(int slotIndex);
void uwamonoFollowSpline(int slotIndex);
void uwamonoUpdateTargetVector(int index);
int mapFindBlockEntry(int scale, short value);
float cursolGetByReduceSpeedAngleLoop(float angle, float target, float step);
float uwamonoRecordsOverlap(int first, int second);
int unduCheckInsideFan(int index, Vector4 *from, Vector4 *to, float facing, float length, float width, float radius, int flags);
void uwamonoBeginAim(int index);
void uwamonoBeginEncounter(int index, signed char flag, int value);

void uwamonoUpdateRecord(int index)
{
    UwamonoRecord *record;
    UwamonoState *state;
    UwamonoActor *actor;
    UwamonoActor *player;

    record = &uwamonoRecords[index];
    actor = uwamonoRecords[index].actor;
    state = uwamonoRecords[index].state;
    player = uwamonoRecords[uwamonoCurrentIndex].actor;
    if (gameWork.control & 1)
    {
        return;
    }
    state->alert = 99;
    uwamonoDrawPointPath(index);
    uwamonoDrawStateLinks(index);
    if (state->noticeCooldown > 0)
    {
        state->noticeCooldown--;
    }
    if (uwamonoCurrentIndex == index)
    {
        return;
    }
    if (record->flags & 0x2000000)
    {
        return;
    }
    if (state->priority == 3)
    {
        uwamonoUpdateTargetVector(index);
    }
    if (((UwamonoFlagsView *) &record->flags)->flags64 & 0xC00000)
    {
        *(void **) &actor->ground = (void *) mapFindBlockEntry(0x100, -1);
        if (record->flags & 0x800000)
        {
            actor->motionSpeed = 0.0f;
        }
        else
        {
            actor->motionSpeed = 1.0f;
        }
        if (record->state->action == UWAMONO_MODE_INIT)
        {
            return;
        }
        if (Get_Distance(&actor->position, &uwamonoRecords[uwamonoCurrentIndex].actor->position) <=
                actor->radius + uwamonoRecords[uwamonoCurrentIndex].actor->radius + D_0069B0F0 &&
            __builtin_fabsf(actor->position.y - uwamonoRecords[uwamonoCurrentIndex].actor->position.y) <
                uwamonoRecords[uwamonoCurrentIndex].form * D_0069B0F4 &&
            gameWork.blocked == 0)
        {
            uwamonoBeginEncounter(index, 0, 0);
        }
        if (record->flags & 0x400)
        {
            actor->rotation.y = record->angle = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 6.0f);
        }
        return;
    }
    if (record->state->action == UWAMONO_MODE_INIT)
    {
        return;
    }
    if ((*(signed char *) &state->mode) != 6 && (*(signed char *) &state->mode) != 9)
    {
        state->flags &= 0xFE;
        if (state->handle != 0)
        {
            sefReleaseRecordHandle(state->handle);
            state->handle = 0;
        }
    }
    switch ((*(signed char *) &state->mode))
    {
    case UWAMONO_MODE_INIT:
        uwamonoInitState(index);
    case UWAMONO_MODE_WANDER:
        uwamonoNextAction(index);
        break;
    case UWAMONO_MODE_WAIT:
        uwamonoTickWait(index);
        break;
    case UWAMONO_MODE_ANCHOR:
        uwamonoOrbitAnchor(index);
        break;
    case UWAMONO_MODE_AIM:
        uwamonoAimAtCurrent(index);
        break;
    case UWAMONO_MODE_PURSUE:
        uwamonoPursueFocus(index);
        break;
    case UWAMONO_MODE_COOLDOWN:
        uwamonoTickCooldown(index);
        break;
    case UWAMONO_MODE_SCRIPT:
        uwamonoStepScript(index);
        break;
    case UWAMONO_MODE_ALERT:
        uwamonoAdvanceTimer(index);
        break;
    case UWAMONO_MODE_ROUTE:
        uwamonoStepRoute(index);
        break;
    case UWAMONO_MODE_SPLINE:
        uwamonoFollowSpline(index);
        break;
    }
    DispPillar(&state->area, state->areaSize, 0.0f, 2);
    if (state->fanLength != 0.0f && !(record->flags & 0x2000000) && state->noticeCooldown <= 0 &&
        uwamonoRecordsOverlap(uwamonoCurrentIndex, index) == 1.0f)
    {
        if (unduCheckInsideFan(index, &record->actor->position, &uwamonoRecords[uwamonoCurrentIndex].actor->position,
                               actor->rotation.y, state->fanLength, state->fanWidth, actor->radius, state->fanFlags) == 1)
        {
            uwamonoBeginAim(index);
        }
        uwamonoDrawFanEdge(&record->position.vector, actor->rotation.y - state->fanWidth * 0.5f, state->fanLength, 1);
        uwamonoDrawFanEdge(&record->position.vector, actor->rotation.y + state->fanWidth * 0.5f, state->fanLength, 1);
    }
    if (pxyTickScaleDouble != 0)
    {
        if ((D_0063EDAA[0] & 4) || gameWork.blocked != 0)
        {
            if ((*(signed char *) &state->mode) == 5)
            {
                uwamonoHaltPursuit(index);
            }
        }
    }
    if (Get_Distance(&actor->position, &player->position) <= actor->radius + player->radius + D_0069B0F8 &&
        __builtin_fabsf(actor->position.y - uwamonoRecords[uwamonoCurrentIndex].actor->position.y) <
            uwamonoRecords[uwamonoCurrentIndex].form * D_0069B0FC &&
        gameWork.blocked == 0)
    {
        uwamonoBeginEncounter(index, 0, 0);
    }
    actor->rotation.y = record->angle = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 6.0f);
    xglFontDebugPrintf(8, 160, D_0069D450, (double) actor->motionSpeed);
}

int HitCheckMapUnitPosSize(const Vector4 *position, float radius);
int uwamonoFindOverlapRecord(int index, Vector4 *position, float scale);
float func_0020A968(int index, Vector4 *position, int motion, int frame, int flags);
extern char D_00689E60[];

/*
 * Steps the record of one slot toward the target along the heading to it, at
 * the given speed. The step is swept against the map; a hit on a map unit or on
 * another record puts the record back where it was. Returns 1 when the record
 * moved freely (the actor then walks or runs by the step length), 0 otherwise.
 */
int uwamonoMoveAlongHeading(int index, Vector4 *target, float speed)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    DoublewordAlignedVector4 *position;
    struct { float x, y, z; } delta;
    Vector4 origin;
    DoublewordAlignedVector4 saved = record->position.vector;
    UwamonoState *state = record->state;
    UwamonoActor *actor = record->actor;
    UwamonoPosition swept;
    Vector4 step;
    float heading;
    float distance;
    int hit;
    float ground;
    float zero = 0.0f;

    origin.z = zero;
    origin.y = zero;
    origin.x = zero;
    origin.w = 1.0f;
    position = &record->position.vector;
    heading = Get_Angle(position, target);
    delta.x = sinf(heading) * speed;
    delta.y = zero;
    delta.z = cosf(heading) * speed;
    step.x = delta.x;
    step.y = delta.y;
    step.z = delta.z;
    step.w = 1.0f;
    distance = Get_Distance(&origin, &step);
    position->x += step.x;
    record->position.vector.w = 1.0f;
    record->position.vector.y += step.y;
    record->position.vector.z += step.z;
    if (unduCheckSweep(index, &actor->position, position, state->fanFlags,
                       state->groundFrame, &swept.vector, 0x303) == 0)
    {
        ((UwamonoWordView *) &actor->direction.y)->value = heading;
        record->position = swept;
        actor->position = record->position.vector;
        actor->position.y = record->position.vector.y =
            func_0020A968(index, position, state->fanFlags,
                          state->groundFrame, 0);
        return 0;
    }
    hit = HitCheckMapUnitPosSize(position, actor->radius);
    if (hit != -1)
    {
        ((UwamonoWordView *) &actor->direction.y)->value = heading;
        record->position.vector = saved;
        actor->position = record->position.vector;
        return 0;
    }
    if (uwamonoFindOverlapRecord(index, position, 1.0f) != hit)
    {
        ((UwamonoWordView *) &actor->direction.y)->value = heading;
        if ((*(signed char *) &state->mode) != 10)
        {
            record->position.vector = saved;
            actor->position = record->position.vector;
        }
        return 0;
    }
    ((UwamonoWordView *) &actor->direction.y)->value = heading;
    record->position = swept;
    actor->position = record->position.vector;
    ground = func_0020A968(index, position, state->fanFlags,
                           state->groundFrame, 0);
    if (ground != -1000.0f)
    {
        record->position.vector.y = ground;
        actor->position.y = ground;
    }
    else
    {
        xglFontDebugPrintf(8, 128, D_00689E60);
    }
    XenoActorEnableChannel(actor);
    uwamonoSetMotion(index, distance <= 0.135f ? 1 : 2, 1.0f);
    return 1;
}

/*
 * Copies the point into the record's position when it lies within 0.1 of the
 * record and the mode is 1. Returns 0 when the point is out of reach, else 1.
 */
int uwamonoCopyPointIfNear(int index, const Vector4 *point, signed char copyFlag)
{
    UwamonoRecord *record = &uwamonoRecords[index];

    if (!(Get_Distance(&record->position.vector, point) < 0.1f))
    {
        return 0;
    }
    if (copyFlag == 1)
    {
        record->position.vector = *point;
    }
    return 1;
}

void Get_Point_By_AngleLength(const Vector4 *source, Vector4 *destination, float angle, float length);

/*
 * Probes the point at the given length ahead of the slot's actor. A ready
 * record (kind 1 or 4) other than the slot's own that overlaps it and lies
 * within the two sizes of the point becomes the target (scan state 0).
 * Otherwise the map is probed at the point: scan state 64 when it finds no
 * ground, else 99. Returns 1 only in the last case.
 */
int uwamonoScanTargets(int slotIndex, float length)
{
    UwamonoSlot *slot;
    UwamonoRecord *slotRecord;
    UwamonoActor *actor;
    UwamonoState *state;
    Vector4 point;
    short i;
    int offset;

    slotRecord = &uwamonoRecords[slotIndex];
    actor = uwamonoRecords[slotIndex].actor;
    state = uwamonoRecords[slotIndex].state;
    Get_Point_By_AngleLength(&actor->position, &point, actor->direction.y, length);
    for (i = 0; i < UWAMONO_RECORD_COUNT; i++)
    {
        if (uwamonoRecords[i].kind == 0 || uwamonoRecords[i].kind == 5)
        {
            continue;
        }
        if (uwamonoRecords[i].kind == 1 || uwamonoRecords[i].kind == 4)
        {
            if (slotIndex != i && uwamonoRecordsOverlap(slotIndex, i) != 0.0f)
            {
                if (Get_Distance(&point, &uwamonoRecords[i].position.vector) <= uwamonoRecords[slotIndex].extent.x + uwamonoRecords[i].extent.x)
                {
                    state->targetRecord = i;
                    state->scanState = UWAMONO_SCAN_RECORD;
                    return 0;
                }
            }
        }
    }
    actor->ground = mapFindBlockEntry(state->fanFlags, state->groundFrame);
    actor->probeRadius = ((UwamonoVectorView *) &slotRecord->extent)->vector.x;
    actor->probeEnabled = 2;
    actor->hitFlags = 0x303;
    if (mapMoveProbe(&point, 0, actor->hitBuffer) == -1000.0f)
    {
        state->scanState = UWAMONO_SCAN_NO_GROUND;
        return 0;
    }
    state->scanState = UWAMONO_SCAN_GROUND;
    return 1;
}

/*
 * Starts a motion on the actor of one slot. A motion below 16 indexes the
 * state's motion table; the value splits into a mode slot (value / 100) and a
 * mode (value % 100). The actor is re-set to the channel value 4.0 and given
 * the mode unless it already holds that slot and mode.
 */
void uwamonoSetMotion(int slotIndex, int motion, float speed)
{
    UwamonoSlot *slot = &uwamonoSlotTable[slotIndex];
    UwamonoActor *actor = slot->actor;
    UwamonoState *state = slot->state;

    if (motion < 16)
    {
        motion = state->motionValues[motion];
    }
    actor->motionSpeed = speed;
    if (actor->modeSlot == motion / 100 && actor->mode == motion % 100)
    {
        return;
    }
    XenoActorSetChannelValue(actor, 4.0f);
    XenoActorSetModeInSlot(actor, motion / 100, motion % 100);
}

#include "main/pad.h"



extern float D_0069B108;
void XenoActorResetChannelFlags(UwamonoActor *actor);
void *sefSpawnEffectRecordById(unsigned int id, int unknown1, int unknown2);
void sefSetRecordOwner(void *record, UwamonoActor *actor);

typedef struct UwamonoEffect {
    unsigned char unmodeled_00[0x20];
    Vector4 scale;                      /* +0x20 */
} UwamonoEffect;

static inline UwamonoRecord *uwamonoRecordAt(int index)
{
    return &uwamonoRecords[index];
}

void uwamonoSetAction(int slot_index, signed char action)
{
    UwamonoRecord *record = uwamonoRecordAt(slot_index);
    UwamonoState *state = uwamonoRecords[slot_index].state;
    UwamonoActor *actor = uwamonoRecords[slot_index].actor;

    if (record->kind != 1)
    {
        return;
    }
    state->mode = action;
    state->timer = 0;
    XenoActorSetChannelValue(actor, 4.0f);
    XenoActorEnableChannel(actor);
    switch (action)
    {
    case UWAMONO_MODE_ANCHOR:
        uwamonoSetMotion(slot_index, 1, 1.0f);
        break;
    case UWAMONO_MODE_WANDER:
        state->timerLimit = state->wanderTime;
        uwamonoSetMotion(slot_index, 0, 1.0f);
        break;
    case UWAMONO_MODE_START:
        if ((s16) state->wanderTime > 0)
        {
            state->timerLimit = state->wanderTime;
            uwamonoSetMotion(slot_index, 0, 1.0f);
            state->mode = 1;
        }
        break;
    case UWAMONO_MODE_WAIT:
        if ((s16) state->waitTime > 0)
        {
            state->timerLimit = state->waitTime;
            uwamonoSetMotion(slot_index, 4, 1.0f);
        }
        break;
    case UWAMONO_MODE_AIM:
        state->timerLimit = 14;
        uwamonoSetMotion(slot_index, 3, 1.0f);
        if (state->blockFlags & 1)
        {
            if (gameWork.releaseHandle == 0)
            {
                gameWork.releaseHandle = sefSpawnEffectRecordById(0x10000021, 0, 0);
            }
        }
        if (state->voiceId[0] != -1)
        {
            seVoiceStartDefault(0x03000000 + state->voiceId[Get_Rnd(0, 2)]);
        }
        break;
    case UWAMONO_MODE_PURSUE:
        state->timerLimit = 30;
        state->field7A = 0;
        actor->flags = (actor->flags | 0x200000) & 0x7FFFFFFF;
        uwamonoSetMotion(slot_index, 2, 1.0f);
        break;
    case UWAMONO_MODE_SCRIPT:
    {
        int motionIndex = state->routeIndex * 4 + state->motionCol;

        state->timerLimit = state->motionTime[motionIndex];
        uwamonoSetMotion(slot_index, state->motionSpeed[motionIndex], 1.0f);
        break;
    }
    case UWAMONO_MODE_ALERT:
    {
        UwamonoEffect *effect;
        float formScale;

        state->flags |= 1;
        (*(s16 *) &state->cooldown) += 300;
        state->timerLimit = 30;
        actor->flags &= 0xFEFFFFFF;
        if (state->handle != 0)
        {
            sefReleaseRecordHandle(state->handle);
        }
        effect = sefSpawnEffectRecordById(0x10000037, 0, 0);
        sefSetRecordOwner(effect, actor);
        state->handle = effect;
        formScale = uwamonoRecords[uwamonoCurrentIndex].form;
        effect->scale.z = formScale;
        effect->scale.y = formScale;
        effect->scale.x = formScale;
        ((UwamonoWordView *) &actor->motionSpeed)->word = 0;
        XenoActorResetChannelFlags(actor);
        break;
    }
    case UWAMONO_MODE_COOLDOWN:
        ((UwamonoWordView *) &actor->motionSpeed)->word = 0;
        break;
    default:
        break;
    }
}

void uwamonoHaltPursuit(void)
{
    int index;
    unsigned char kind;
    UwamonoState *state;
    Vector4 *route;
    UwamonoActor *actor;

    for (index = 0; index < 256; index++) {
        kind = uwamonoRecords[index].kind;
        if (kind != 1) {
            continue;
        }
        state = uwamonoRecords[index].state;
        route = state->route;
        actor = uwamonoRecords[index].actor;
        if (((signed char) state->mode) == 6) {
            (*(s16 *) &state->cooldown) = kind;
        }
        if (((signed char) state->mode) != 5 && ((signed char) state->mode) != 9) {
            continue;
        }
        state->step = 0;
        vectorCopyPoint(route, &actor->position, actor->direction.y);
        uwamonoSetAction(index, 2);
        uwamonoRebuildTrail(index);
    }
}

int uwamonoFindReadyInReach(const Vector4 *from, int index)
{
    short i;

    for (i = 0; i < UWAMONO_RECORD_COUNT; i++) {
        UwamonoRecord *record = &uwamonoRecords[i];
        UwamonoState *state = record->state;
        float distance = Get_Distance(from, &record->position.vector);

        if (record->kind == 1) {
            if (!(((&uwamonoRecords[index])->state->blockFlags & 1) && i == uwamonoCurrentIndex)) {
                if (!((state->blockFlags & 1) && index == uwamonoCurrentIndex)) {
                    if (record->extent.x != 0.0f && !(record->flags & 2) && index != i
                        && !(__builtin_fabsf(record->position.vector.y - from->y) >= 1.0f)
                        && distance <= (uwamonoRecords[index].extent.x + record->extent.x) + D_0069B108) {
                        break;
                    }
                }
            }
        }
    }
    return i == UWAMONO_RECORD_COUNT ? -1 : i;
}

int uwamonoAnyEngaged(void)
{
    short i;

    for (i = 0; i < UWAMONO_RECORD_COUNT; i++) {
        UwamonoState *state = uwamonoRecords[i].state;

        if (uwamonoRecords[i].kind == 1 && (state->blockFlags & 1) && (unsigned) (state->mode - 4) < 2) {
            return 1;
        }
    }
    if (gameWork.releaseHandle != 0) {
        sefReleaseRecordHandle(gameWork.releaseHandle);
        gameWork.releaseHandle = 0;
    }
    return 0;
}

float Get_Distance3D(const Vector4 *first, const Vector4 *second);

void uwamonoNoticePlayer(int viewerIndex, const Vector4 *viewpoint)
{
    int i;
    UwamonoRecord *record = uwamonoRecords;
    UwamonoState *state;
    UwamonoActor *actor;
    Vector4 hit;

    for (i = 0; i < UWAMONO_RECORD_COUNT; i++, record++) {
        if (viewerIndex == i) {
            continue;
        }
        if (record->kind != 1) {
            continue;
        }
        state = record->state;
        actor = record->actor;
        if (state->noticeRange == 0.0f || (record->flags & 0x800000) || state->noticeCooldown > 0) {
            continue;
        }
        if (unduCheckSweep(i, &actor->position, viewpoint, state->fanFlags, state->groundFrame, &hit, 0)) {
            if (Get_Distance3D(viewpoint, &actor->position) < state->noticeRange) {
                uwamonoBeginAim(i);
            }
        }
    }
}



extern PadRecord PadData[];

void uwamonoBeginAim(int index)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    UwamonoState *state;
    UwamonoActor *actor;
    int focus;

    state = record->state;
    if (record->actor->kind == 0) {
        return;
    }
    actor = record->actor;
    if (state->blockFlags < 0 && state->field78 < 0) {
        return;
    }
    if (record->flags & 0x2000000) {
        return;
    }
    if (gameWork.blocked != 0 || uwamonoLadderState != 0 || gameWork.field06 != 0) {
        return;
    }
    if ((gameWork.control & 0x20) && gameWork.field1C != -1) {
        return;
    }
    if ((unsigned) (state->mode - 4) < 2) {
        return;
    }
    if ((signed char) state->mode == 9 || (signed char) state->mode == 6 || (signed char) state->mode == 0
        || (signed char) state->mode == 10) {
        return;
    }
    actor->flags |= 0x400000;
    if (pxyTickScaleDouble != 0 && (PadData[0].held & 0x200)) {
        return;
    }
    if (state->action == UWAMONO_MODE_COOLDOWN || state->action == UWAMONO_MODE_SPLINE_START) {
        return;
    }
    uwamonoSetAction(index, 4);
    state->focusIndex = (s16) uwamonoCurrentIndex;
    focus = uwamonoCurrentIndex;
    state->homePoint = uwamonoRecords[focus].position.vector;
}

extern u8 D_0069D1E4;
float Get_Angle_Relative(const Vector4 *first, const Vector4 *second, float reference);
int unduCheckAngle(float angle, float lo, float hi);
int Get_Rnd(int min, int max);

void uwamonoBeginEncounter(int index, signed char takeValue, int reactionValue)
{
    short sectors[2];
    unsigned char reactionTable[3][3] = { { 0, 3, 4 }, { 1, 3, 4 }, { 2, 3, 4 } };
    short candidateCount = 0;
    short i;
    UwamonoSlot *slot;
    UwamonoState *state;
    UwamonoActor *actor;
    float angle;
    UwamonoGameWork *work;
    int inSector;
    int slotOffset;

    if (index == -1) {
        gameWork.encounterRecord = index;
        gameWork.flags |= 1;
        gameWork.encounterFlags = reactionValue;
        gameWork.encounterTarget = 0;
        gameWork.encounterReaction = 0;
        return;
    }
    slotOffset = index * sizeof(UwamonoRecord) + 0x330;
    slot = (UwamonoSlot *) ((char *) uwamonoRecords + slotOffset);
    state = slot->state;
    actor = slot->actor;
    if (uwamonoLadderState != 0) {
        return;
    }
    if (takeValue == 0) {
        if (!(state->blockFlags & 1)) {
            return;
        }
        if (gameWork.control & 0x8000) {
            return;
        }
        if (gameWork.field06 != 0) {
            return;
        }
        if ((gameWork.control & 0x20) && gameWork.field1C != -1) {
            return;
        }
        angle = Get_Angle_Relative(&uwamonoRecords[uwamonoCurrentIndex].position.vector, &actor->position,
                                   uwamonoRecords[uwamonoCurrentIndex].angle);
        for (;;) {
            if (unduCheckAngle(angle, -1.22173059f, 1.22173059f) == 1) {
                sectors[0] = 0;
                break;
            }
            inSector = unduCheckAngle(angle, 1.22173059f, 2.61799383f);
            if (inSector == 1) {
                sectors[0] = inSector;
                break;
            }
            inSector = unduCheckAngle(angle, -2.61799383f, 1.22173059f);
            if (inSector == 1) {
                sectors[0] = inSector;
            } else {
                sectors[0] = 2;
            }
            break;
        }
        angle = Get_Angle_Relative(&actor->position, &uwamonoRecords[uwamonoCurrentIndex].position.vector,
                                   actor->rotation.y);
        for (;;) {
            if (unduCheckAngle(angle, -1.22173059f, 1.22173059f) != 1) {
                inSector = unduCheckAngle(angle, 1.22173059f, 2.61799383f);
                if (inSector == 1) {
                    sectors[1] = inSector;
                    break;
                }
                inSector = unduCheckAngle(angle, -2.61799383f, 1.22173059f);
                if (inSector == 1) {
                    sectors[1] = inSector;
                } else {
                    sectors[1] = 2;
                }
                break;
            }
            sectors[1] = 0;
            break;
        }
        state->reactionKind = reactionTable[sectors[0]][sectors[1]];
        gameWork.encounterReaction = (signed char) reactionTable[sectors[0]][sectors[1]];
    } else {
        gameWork.encounterReaction = reactionValue;
    }
    for (i = 0; i < 8; i++) {
        if ((short) state->candidates[i] != -1) {
            candidateCount++;
        }
    }
    work = &gameWork;
    if (candidateCount > 0) {
        work->encounterTarget = state->candidates[Get_Rnd(0, candidateCount - 1)];
    } else {
        work->encounterTarget = 0;
    }
    gameWork.encounterRecord = index;
    gameWork.flags |= 1;
    gameWork.encounterFlags = 0;
    if (uwamonoRecords[uwamonoCurrentIndex].form != 1.0f) {
        gameWork.encounterFlags = 0x800;
    }
    if ((char) state->flags & 1) {
        if (D_0069D1E4 == 1) {
            gameWork.encounterFlags |= 1;
        } else {
            gameWork.encounterFlags |= 2;
        }
    }
    if (gameWork.field1B20 == 0) {
        gameWork.encounterFlags |= 0x20;
    }
    gameWork.encounterFlags |= state->field4064;
    if (state->handle != 0) {
        sefReleaseRecordHandle(state->handle);
        state->handle = 0;
    }
    if (gameWork.releaseHandle != 0) {
        sefReleaseRecordHandle(gameWork.releaseHandle);
        gameWork.releaseHandle = 0;
    }
}

void uwamonoCompactRoute(int index, short step)
{
    UwamonoState *state = uwamonoSlotTable[index].state;
    short i;

    if (state->action == UWAMONO_MODE_SCRIPT || state->action == UWAMONO_MODE_START) {
        for (i = 0; i < state->pendingCount; i++) {
            state->routeRows[i].f[0] = state->pending[i].f[0];
            state->routeRows[i].f[1] = state->pending[i].f[1];
            state->routeRows[i].f[2] = state->pending[i].f[2];
        }
        state->routeCount = state->pendingCount;
        state->step = (s16) ((state->routeCount + step) * 128 + 2);
    }
}

int Check_Straight_ID(float radius, const Vector4 *from, const Vector4 *to, int limit, int *out, int mask);
void uwamonoPushTrailPoint(int index, float x, float y, float z);

void uwamonoRebuildTrail(int index)
{
    UwamonoSlot *slot = &uwamonoSlotTable[index];
    UwamonoState *state = slot->state;
    UwamonoActor *actor = slot->actor;
    float pointDistance[64];
    short pathOrder[64];
    int straightHits[4];
    short j;
    short i;
    short k;
    short current = 0;
    short pathLength;
    int guard;
    float angle;
    float best;

    state->noticeCooldown = 60;
    if (!((s16) state->pointCount > 0)) {
        if (state->action != 6) {
            state->action = UWAMONO_MODE_WANDER;
            actor->flags &= 0x7FFFFFFF;
        }
        return;
    }
    for (k = 0; k < (s16) state->pointCount; k++) {
        pointDistance[k] = Get_Distance(&actor->position, &state->points[k].v);
    }
    for (i = 0; i < (s16) (s16) state->pointCount; i++) {
        best = 1000.0f;
        pathLength = -1;
        for (j = 0; j < (s16) state->pointCount; j++) {
            if (pointDistance[j] < best && 0.0f < pointDistance[j]) {
                best = pointDistance[j];
                pathLength = j;
            }
        }
        pathOrder[i] = pathLength;
        pointDistance[pathLength] = 0.0f;
    }
    for (i = 0; i < (s16) state->pointCount; i++) {
        if (Check_Straight_ID(actor->radius, &actor->position, &state->points[pathOrder[i]].v, 32, straightHits, 2048) == 32) {
            current = pathOrder[i];
            break;
        }
    }
    pathOrder[0] = current;
    guard = 0;
    pathLength = 1;
    while (current != 0) {
        for (i = 0; i < state->linkCount[current]; i++) {
            if (state->links[current][i] < current) {
                current = state->links[current][i];
            }
        }
        guard++;
        if (current == 0) {
            pathOrder[pathLength++] = 0;
            break;
        }
        if (guard > 0xFFFE) {
            break;
        }
        pathOrder[pathLength++] = current;
    }
    angle = Get_Angle(&actor->position, &state->points[pathOrder[i]].v);
    actor->direction.y = angle;
    vectorCopyPoint(&state->route[0], &actor->position, angle);
    state->routeCount = 1;
    state->step = 1;
    for (i = 0; i < pathLength; i++) {
        uwamonoPushTrailPoint(index, state->points[pathOrder[i]].f[0], state->points[pathOrder[i]].f[1], state->points[pathOrder[i]].f[2]);
    }
    Get_Angle(&state->route[state->routeCount - 2], &state->route[state->routeCount - 1]);
    actor->flags |= 0x80000000;
    uwamonoSetAction(index, 3);
    uwamonoSetAction(index, 2);
}
