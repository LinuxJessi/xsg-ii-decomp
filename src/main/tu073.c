#include "common.h"
#include "shared.h"
#include "main/xenoActor.h"
#include "main/tu086.h"

int uwamonoFindIndexByShortKey(short key);
void func_A9CC70(int call, int error_code);
/* The running native call's argument count; it reads the call record
 * (src/ov01/tu046.c). With the call declared, enepcLookAt schedules
 * `daddu $18,$4` into the jal delay slot with $4 still live; declared without
 * it, the copy of call moves ahead of the array init. */
int func_A9F0A8(int call);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);

/* The XenoActor of a map object (offsets as in include/main/tu051.h); only
 * the fields used below. The resource pointer at +0x600 is written by
 * enepcExMotion. */
struct XenoActor {
    unsigned char unmodeled_0000[0x10];
    Vector4 position; /* +0x10 */
    unsigned char unmodeled_0020[0x60 - 0x20];
    /* A one-member union: Resigne reads it with alias set 0, after the short
     * store before it. */
    union {
        Vector4 direction; /* +0x60: y is the moving angle (include/main/tu051.h) */
    };
    unsigned char unmodeled_0070[0xBC - 0x70];
    u32 drawFlags; /* +0xBC: 0x4 hides the actor (include/main/tu051.h); u32 as in xenoActor.c */
    unsigned char unmodeled_00c0[0x5E4 - 0xC0];
    int cachedSelector; /* +0x5E4: the last XenoActorSetSelector value */
    unsigned char unmodeled_05e8[0x600 - 0x5E8];
    void *resource; /* +0x600 */
};

/* The per-unit state block pointed to by uwamonoSlotTable[].state,
 * uwamonoStateSlots[].state and uwamonoRecords[].motions (same 0x870-byte stride).
 * Only the fields used by the functions below are modeled. */
struct UwamonoState {
    unsigned char unmodeled_0000[0xC];
    signed char action; /* +0x0C: 5 while resigned, see Resigne */
    unsigned char unmodeled_000d[0x20 - 0xD];
    /* A one-member union: enepcAlgorithm stores it with alias set 0, the
     * state pointer is reloaded after each store. */
    union {
        Vector4 home; /* +0x20: the record position, written by enepcAlgorithm */
    };
    float speed; /* +0x30 */
    unsigned char unmodeled_0034[0x58 - 0x34];
    short trailStep; /* +0x58: reset with the trail (uwamonoRebuildTrail) */
    unsigned char unmodeled_005a[0x5F - 0x5A];
    unsigned char mode; /* +0x5F, written by enepcAlgorithm */
    unsigned char unmodeled_0060[0x90 - 0x60];
    Vector4 trail[256]; /* +0x90: trailCount at +0x1090 */
    unsigned char unmodeled_1090[0x4010 - 0x1090];
    int seIds[3]; /* +0x4010, written by enepcSetSe */
    unsigned char unmodeled_401c[0x4042 - 0x401C];
    short groups[8]; /* +0x4042, written by setGroup */
    unsigned char unmodeled_4052[0x4054 - 0x4052];
    int together[4]; /* +0x4054, written by enepcSetTogether */
    short battle; /* +0x4064: native setBattle */
    unsigned char unmodeled_4066[0x4074 - 0x4066];
    int resourceId; /* +0x4074 */
    int walk[16]; /* +0x4078: written by enepcDefaultMotion */
};

/* One 0x870-byte slot of uwamonoSlotTable; only the actor and state pointers
 * at +0x00 and +0x04 are modeled here (same fields as src/main/tu085.c). */
struct UwamonoSlot {
    struct XenoActor *actor; /* +0x00 */
    struct UwamonoState *state; /* +0x04 */
    unsigned char unmodeled_08[0x870 - 8];
};

extern struct UwamonoSlot uwamonoSlotTable[];
int GameResLoadID(int id, void **buffer, unsigned int *size,
                  void (*callback)(int index, void *buffer, unsigned int size, int value));

/* One 0x870-byte record of uwamonoRecords (include/main/tu051.h). The actor
 * and state pointers at +0x330/+0x334 are the same fields uwamonoSlotTable[]
 * names actor and state. */
typedef struct UwamonoRecord {
    unsigned char unmodeled_000[2];
    unsigned char kind; /* +0x002: MAP_KIND_* */
    unsigned char unmodeled_003[0x10 - 3];
    float position[4]; /* +0x010: read as an array by enepcAlgorithm */
    unsigned char unmodeled_020[0x40 - 0x20];
    AlignedVector4 rotation; /* +0x040: radians (tu067.c); quadword-aligned, makes the record 16-aligned */
    unsigned char unmodeled_050[0x330 - 0x50];
    struct XenoActor *actor; /* +0x330 */
    struct UwamonoState *motions; /* +0x334 */
    unsigned char unmodeled_338[0x870 - 0x338];
} UwamonoRecord;

/* UwamonoRecord.kind (the names of tu050.c's MapObjectRecord.kind). */
enum {
    MAP_KIND_FREE = 0,
    MAP_KIND_ACTOR = 1,
    MAP_KIND_MODEL = 2,
    MAP_KIND_FLAG = 3,
    MAP_KIND_ACTOR_STATIC = 4, /* an actor that is not moved by uwamonoUpdateRecord */
    MAP_KIND_PART = 5,         /* a part attached to the player (tu051.c) */
    MAP_KIND_EFFECT = 6,
    MAP_KIND_MOVER = 7,
    MAP_KIND_MODEL_REQUEST = 8 /* create only: uwamonoCreateRecord turns it into a model or frees it */
};

#define MAP_RECORD_COUNT 0x100

extern UwamonoRecord uwamonoRecords[];

float XenoActorSetSelector(struct XenoActor *actor, int selector);

/*
 * Native "motionFace". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 3 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found and the record is of kind
 * MAP_KIND_ACTOR or MAP_KIND_ACTOR_STATIC and its actor's selector differs from argument 2, sets the actor's channel
 * value to 4.0f and its selector to argument 2. Argument 3 is read but
 * unused. Returns 1.
 */
int motionFace(int call)
{
    int lookupIndex;
    int selector;
    struct XenoActor *actor;

    if (func_A9F0A8(call) < 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    if (lookupIndex == -1) {
        return 1;
    }
    selector = func_A9F0C0(call, 2);
    /* Argument 3 is read and discarded: the original calls func_A9F0C0(call, 3)
     * and never uses the result. */
    func_A9F0C0(call, 3);
    if (uwamonoRecords[lookupIndex].kind != MAP_KIND_ACTOR
        && uwamonoRecords[lookupIndex].kind != MAP_KIND_ACTOR_STATIC) {
        return 1;
    }
    actor = uwamonoRecords[lookupIndex].actor;
    if (actor->cachedSelector != selector) {
        XenoActorSetChannelValueB(actor, 4.0f);
        XenoActorSetSelector(actor, selector);
    }
    return 1;
}

/*
 * Native "Extra". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 2 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; when found and the slot has an actor, passes
 * func_A9F0C0's second argument to XenoActorSetFirstWord for the actor.
 * Returns 1 on success, 0 otherwise.
 */
int enepcExtra(int call)
{
    int lookupKey;
    int lookupIndex;
    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    /* The original reads argument 2 here, discards it, and reads it again for
     * the XenoActorSetFirstWord call below (two jal func_A9F0C0 with $5 = 2). */
    func_A9F0C0(call, 2);
    if (lookupIndex == -1) {
        return 0;
    }
    if (uwamonoSlotTable[lookupIndex].actor == 0) {
        return 0;
    }
    XenoActorSetFirstWord(uwamonoSlotTable[lookupIndex].actor, func_A9F0C0(call, 2));
    return 1;
}

/*
 * Native "ExMotion". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 2. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, loads the resource for func_A9F0C0's
 * second argument, stores the resource id to the slot's state and the loaded
 * resource to the slot's actor. Returns 1.
 */
int enepcExMotion(int call)
{
    void *resource;
    int lookupKey;
    int lookupIndex;
    int resourceId;
    struct UwamonoSlot *slot;

    resource = 0;
    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    if (lookupIndex == -1) {
        return 1;
    }
    resourceId = func_A9F0C0(call, 2);
    GameResLoadID(resourceId, &resource, 0, 0);
    slot = &uwamonoSlotTable[lookupIndex];
    slot->actor->resource = resource;
    slot->state->resourceId = resourceId;
    return 1;
}

/* Candidate target points and their reach (10 entries; src/main/tu051.c reads
 * them as Vector4 D_0033EE00[]; the float[][4] view gives this function's
 * point-index addressing, the Vector4 one swaps the addu operands). */
extern float D_0033EE00[][4];
extern float D_0033EE90[];

int uwamonoTryClearTarget(int index, signed char threshold);

/*
 * Native "lookat". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 4 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey. With argument 5 present, when one of the 10
 * candidate target points has no reach yet, sets point number argument 5 to
 * arguments 2 to 4 / 100 with reach argument 6 / 100. Without it: arguments
 * 2 to 4 all omitted clear the record's target; arguments 3 and 4 omitted
 * offer the record found for argument 2 as the target, otherwise the point
 * arguments 2 to 4 / 100. Only records of kind MAP_KIND_ACTOR or
 * MAP_KIND_ACTOR_STATIC take an offer (0 is returned for the others).
 * Returns 1.
 */
int enepcLookAt(int call)
{
    int indices[2] = { -1, -1 };
    Vector4 point;
    int pointIndex;
    float reach;
    int i;

    if (func_A9F0A8(call) < 4) {
        func_A9CC70(call, 2);
        return 0;
    }
    indices[0] = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    if (indices[0] == -1) {
        return 1;
    }
    if (func_A9F1C0(call, 5) == 0) {
        pointIndex = func_A9F0C0(call, 5);
        reach = (float) func_A9F0C0(call, 6) / 100.0f;
        for (i = 0; i < 10; i++) {
            if (D_0033EE90[i] == 0.0f) {
                D_0033EE00[pointIndex][0] = (float) func_A9F0C0(call, 2) / 100.0f;
                D_0033EE00[pointIndex][1] = (float) func_A9F0C0(call, 3) / 100.0f;
                D_0033EE00[pointIndex][2] = (float) func_A9F0C0(call, 4) / 100.0f;
                D_0033EE90[pointIndex] = reach;
                break;
            }
        }
        return 1;
    }
    if (func_A9F1C0(call, 2) != 0 && func_A9F1C0(call, 3) != 0 && func_A9F1C0(call, 4) != 0) {
        uwamonoTryClearTarget(indices[0], 3);
        return 1;
    }
    if (func_A9F1C0(call, 3) != 0 && func_A9F1C0(call, 4) != 0) {
        indices[1] = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 2));
    } else {
        indices[1] = -1;
        point.x = (float) func_A9F0C0(call, 2) / 100.0f;
        point.y = (float) func_A9F0C0(call, 3) / 100.0f;
        point.z = (float) func_A9F0C0(call, 4) / 100.0f;
    }
    if (uwamonoRecords[indices[0]].kind != MAP_KIND_ACTOR
        && uwamonoRecords[indices[0]].kind != MAP_KIND_ACTOR_STATIC) {
        return 0;
    }
    if (indices[1] == -1) {
        uwamonoOfferTarget(indices[0], 3, -1, &point);
    } else {
        uwamonoOfferTarget(indices[0], 3, indices[1], 0);
    }
    return 1;
}

void uwamonoBeginEncounter(int, int, int);

/*
 * Native "Encount". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 2. Otherwise passes the record index of argument 1 (from
 * uwamonoFindIndexByShortKey), 1 and argument 2 to uwamonoBeginEncounter and
 * returns 1.
 */
int enepcEncount(int call)
{
    int key;
    int index;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey(key);
    uwamonoBeginEncounter(index, 1, func_A9F0C0(call, 2));
    return 1;
}

/* src/main/get.c defines it with two parameters; this caller passes the
 * actor's moving angle in $f12 as a third. */
void vectorCopyPoint(Vector4 *destination, const Vector4 *source, float angle);
void uwamonoSetAction(int slot_index, int action);
void uwamonoRebuildTrail(int slot_index);

/*
 * Native "Resigne". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 1. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found and the slot's state is in action 5,
 * clears the trail step, copies the actor position to the first trail point
 * and enters action 2 with a rebuilt trail. Returns 1.
 */
int Resigne(int call)
{
    int lookupIndex;
    struct UwamonoSlot *slot;
    struct UwamonoState *state;
    struct XenoActor *actor;

    if (func_A9F0A8(call) != 1) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    if (lookupIndex == -1) {
        return 1;
    }
    slot = &uwamonoSlotTable[lookupIndex];
    state = slot->state;
    actor = slot->actor;
    if (state->action == 5) {
        state->trailStep = 0;
        vectorCopyPoint(&state->trail[0], &actor->position, actor->direction.y);
        uwamonoSetAction(lookupIndex, 2);
        uwamonoRebuildTrail(lookupIndex);
    }
    return 1;
}

/*
 * Native "setGroup". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 2 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, for each of the 8 entries of the
 * slot's state groups[], stores -1 when func_A9F1C0's argument i+2 is
 * nonzero, otherwise stores func_A9F0C0's argument i+1. Returns 1.
 */
int setGroup(int call)
{
    int lookupKey;
    int lookupIndex;
    int i;
    struct UwamonoSlot *slot;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    if (lookupIndex == -1) {
        return 1;
    }
    slot = &uwamonoSlotTable[lookupIndex];
    for (i = 0; i < 8; i++) {
        if (func_A9F1C0(call, i + 2) != 0) {
            slot->state->groups[i] = -1;
        } else {
            slot->state->groups[i] = (short) func_A9F0C0(call, i + 1);
        }
    }
    return 1;
}

/* One 0x870-byte record of uwamonoStateSlots (uwamonoRecords + 0x334), starting with
 * its state block pointer (same layout as src/main/tu086.c). */
struct UwamonoStateSlot {
    struct UwamonoState *state;
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoStateSlot uwamonoStateSlots[];

/*
 * Native "setBattle". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 2 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's second argument
 * as the state's battle value. Returns 1.
 */
int enepcSetBattle(int call)
{
    int lookupKey;
    int lookupIndex;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    if (lookupIndex == -1) {
        return 1;
    }
    uwamonoStateSlots[lookupIndex].state->battle = func_A9F0C0(call, 2);
    return 1;
}

/*
 * Native "setSE". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 4 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's arguments 2
 * through 4 into the slot's state seIds[0..2]. Returns 1.
 */
int enepcSetSe(int call)
{
    int lookupKey;
    int lookupIndex;
    int i;
    struct UwamonoSlot *slot;

    if (func_A9F0A8(call) < 4) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    if (lookupIndex == -1) {
        return 1;
    }
    slot = &uwamonoSlotTable[lookupIndex];
    for (i = 0; i < 3; i++) {
        slot->state->seIds[i] = func_A9F0C0(call, i + 2);
    }
    return 1;
}

/*
 * Native "defaultMotion". Returns 0 after func_A9CC70 when func_A9F0A8 does
 * not report 3 or more. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's third argument
 * (the motion) into the state's walk[] at the slot given by the second
 * argument. Returns 1.
 */
int enepcDefaultMotion(int call)
{
    int lookupKey;
    int lookupIndex;
    int slot;
    int motion;

    if (func_A9F0A8(call) < 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    if (lookupIndex == -1) {
        return 1;
    }
    slot = func_A9F0C0(call, 2);
    motion = func_A9F0C0(call, 3);
    uwamonoStateSlots[lookupIndex].state->walk[slot] = motion;
    return 1;
}

/*
 * Native "setTogether". Returns 0 after func_A9CC70 when func_A9F0A8 does
 * not report 2 or more. Otherwise resolves script arguments 1 through 4 with
 * uwamonoFindIndexByShortKey (-1 when omitted) and stores the four indices
 * into the together[] of every record found. Returns 1.
 */
int enepcSetTogether(int call)
{
    int members[4];
    int i;
    int j;
    struct UwamonoState *state;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    members[0] = func_A9F1C0(call, 1) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    members[1] = func_A9F1C0(call, 2) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 2));
    members[2] = func_A9F1C0(call, 3) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 3));
    members[3] = func_A9F1C0(call, 4) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 4));
    for (i = 0; i < 4; i++) {
        if (members[i] != -1) {
            state = uwamonoRecords[members[i]].motions;
            for (j = 0; j < 4; j++) {
                state->together[j] = members[j];
            }
        }
    }
    return 1;
}

void uwamonoInitState(int slot_index);

/*
 * Native "Algorithm". Returns 0 after func_A9CC70 when func_A9F0A8 does not
 * report 3. Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores argument 2 as the state's mode, the record position as its
 * home, action 1 and argument 3 / 100 as its speed (1000.0f when omitted),
 * then calls uwamonoInitState. Returns 1.
 */
int enepcAlgorithm(int call)
{
    int lookupIndex;
    int mode;
    float speed;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    if (lookupIndex == -1) {
        return 1;
    }
    mode = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 3) == 0) {
        speed = (float) func_A9F0C0(call, 3) / 100.0f;
    } else {
        speed = 1000.0f;
    }
    uwamonoRecords[lookupIndex].motions->mode = mode;
    uwamonoRecords[lookupIndex].motions->home.x = uwamonoRecords[lookupIndex].position[0];
    uwamonoRecords[lookupIndex].motions->home.y = uwamonoRecords[lookupIndex].position[1];
    uwamonoRecords[lookupIndex].motions->home.z = uwamonoRecords[lookupIndex].position[2];
    uwamonoRecords[lookupIndex].motions->speed = speed;
    uwamonoRecords[lookupIndex].motions->action = 1;
    uwamonoInitState(lookupIndex);
    return 1;
}

extern struct EnepcParamSet *enepcParamSets;

/*
 * Native "readyEnepcData". Returns 0 after func_A9CC70 when func_A9F0A8 does
 * not report 1. Otherwise loads the resource for argument 1 (the preset
 * table that setParams indexes), stores it to enepcParamSets and returns 1.
 */
int enepcReadyEnepcData(int call)
{
    void *resource;
    int resourceId;

    resource = 0;
    if (func_A9F0A8(call) != 1) {
        func_A9CC70(call, 2);
        return 0;
    }
    resourceId = func_A9F0C0(call, 1);
    /* Retries until the resource has loaded. */
    while (GameResLoadID(resourceId, &resource, 0, 0) == -1) {
    }
    enepcParamSets = resource;
    return 1;
}

/*
 * Native "enableDrawFlags". Returns 0 after func_A9CC70 when func_A9F0A8
 * does not report 2 or more. Otherwise looks up func_A9F0C0's first argument
 * with uwamonoFindIndexByShortKey; if found, ORs argument 2 into the draw
 * flags of the actors of kind MAP_KIND_ACTOR_STATIC or MAP_KIND_ACTOR: of
 * every record whose kind equals argument 3,
 * or of the looked-up record alone when argument 3 is omitted. Returns 1.
 */
int enepcEnableDrawFlags(int call)
{
    int lookupIndex;
    int flags;
    int kind;
    int start;
    int end;
    int i;
    struct XenoActor *actor;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 1));
    if (lookupIndex == -1) {
        return 1;
    }
    flags = func_A9F0C0(call, 2);
    kind = func_A9F1C0(call, 3) != 0 ? -1 : func_A9F0C0(call, 3);
    if (kind == -1) {
        start = lookupIndex;
        end = start + 1;
        /* Cannot be taken (lookupIndex was checked above), but the original
         * compares start with -1 again here (bne at 0x001F6CD8). */
        if (start == -1) {
            return 1;
        }
    } else {
        start = 0;
        end = MAP_RECORD_COUNT;
    }
    for (i = start; i < end; i++) {
        if (uwamonoRecords[i].kind == kind || kind == -1) {
            if (uwamonoRecords[i].kind == MAP_KIND_ACTOR_STATIC || uwamonoRecords[i].kind == MAP_KIND_ACTOR) {
                actor = uwamonoRecords[i].actor;
                if (actor != 0) {
                    actor->drawFlags |= flags;
                }
            }
        }
    }
    return 1;
}
