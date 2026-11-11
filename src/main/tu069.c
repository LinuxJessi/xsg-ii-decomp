#include "common.h"
#include "shared.h"
#include "main/tu050.h"
#include "main/tu165.h"
#include "main/xgl_flags.h"

/* Script natives: argument count, n-th argument, nonzero when argument n is omitted, wrong-count error. */
int func_A9F0A8(int call);
void func_A9CC70(int call, int error_code);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);
int uwamonoFindIndexByShortKey(short key);
void uwamonoSetRoboMode(signed char id);
void fadeStartColorAlpha(int frames, float red, float green, float blue, float alpha);
int xglFlagsGet(int bit_offset, int bit_count);
void fadeStartOut(int frames);
void uwamonoSetTransition(signed char mode, int state, int stateArg, int timer);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

/* The talk/elevator block of each 0x870-byte uwamonoRecords record (D_00340350 = uwamonoRecords + 0x590),
   indexed by uwamonoFindIndexByShortKey.  Only the setElevator fields are modeled. */
typedef struct IndexedRecord {
    unsigned char unmodeled_0[2];
    short elevatorMotion;           /* record+0x592 */
    int elevatorArg;                /* record+0x594: setElevator argument 3 */
    unsigned char unmodeled_8[0x870 - 8];
} IndexedRecord;

/* The same 0x870-byte stride, read by progEffect (progEffectTable = uwamonoRecords + 0x638):
   each record's effect (the sef effect record tu067.c and tu050.c type as void *), passed to
   sefExecEffectFrames, whose id parameter tu165.c declares int. */
typedef struct ProgEffectEntry {
    void *effect;
    unsigned char unmodeled_4[0x870 - 4];
} ProgEffectEntry;

extern IndexedRecord D_00340350[];
extern ProgEffectEntry progEffectTable[];
extern int D_003E7AF8[];

/* A bit of gameWork.control; tu051.h cannot be included here, its record types clash with this file's. */
#define CONTROL_SCRIPTED 0x8000 /* include/main/tu051.h */

/* The game work block (tu067.c's GameWork); only the fields these natives touch. */
typedef struct GameWork {
    unsigned char unmodeled_0[8];
    int control;                    /* +0x8: CONTROL_SCRIPTED while a script controls the player */
    unsigned char unmodeled_c[0x14 - 0xC];
    int state;                      /* +0x14: takes the record's eventState when a state change runs at once */
    unsigned char unmodeled_18[0x1C - 0x18];
    int stateArg;                   /* +0x1C: takes the record's eventStateArg */
    unsigned char unmodeled_20[0xC0 - 0x20];
    float color[3];
    unsigned char unmodeled_cc[0x348 - 0xCC];
    float fogDist[4];               /* +0x348: setFog args 2-5 / 100: near, far, min, max for pxySetFogDist */
    unsigned char unmodeled_358[0x360 - 0x358];
    float fogColor[3];              /* +0x360: setFog args 6-8, the color for pxySetFogColor */
    unsigned char unmodeled_36c[0x370 - 0x36C];
    int shadowCasterMode;           /* +0x370: setItai argument 2 (default 1); tu048 passes it each frame to
                                       func_001BA358, the mode pxyDrawShadowCaster tests (1, 2) */
    unsigned char unmodeled_374[0x384 - 0x374];
    float lightDirection[16][3][4]; /* +0x384: per light set, light types 1-3 */
    float lightColor[16][3][4];     /* +0x684 */
    float ambientColor[16][4];      /* +0x984: light type 0 */
    float type4Direction[16][4];    /* +0xA84: light type 4; w 0 disables it */
    float type4Value[16];           /* +0xB84: light type 4, 0.2 by default */
} GameWork;

extern GameWork gameWork;

/* A bit of XenoActor.drawFlags (defined in src/main/xenoActor.c, not in a header). */
#define XENO_DRAW_COLOR 0x1 /* apply the color block: set by setFog after it writes fogDist/fogColor */

/* The XenoActor fields (xenoActor.c names) the fog and draw natives write. */
struct XenoActor {
    unsigned char unmodeled_000[0xBC];
    u32 drawFlags;                  /* 0x0BC */
    unsigned char unmodeled_0c0[0xD0 - 0xC0];
    int renderFlags;                /* 0x0D0 */
    unsigned char unmodeled_0d4[0x180 - 0xD4];
    float modeValue4;               /* 0x180 */
    unsigned char unmodeled_184[0x490 - 0x184];
    float fogDist[4];               /* 0x490: setFog args 2-5 / 100 (xenoActor.c still names it color: asked) */
    float fogColor[3];              /* 0x4A0: setFog args 6-8 (xenoActor.c still names it colorBlock: asked) */
    unsigned char unmodeled_4ac[0x590 - 0x4AC];
    unsigned char colorIndex[16];   /* 0x590: 0xFF marks a free entry */
};

/* One 0x870-byte slot of uwamonoSlotTable; only the actor pointer at +0x00 is
   read here (the same field src/main/tu073.c and tu085.c model). */
typedef struct UwamonoSlot {
    struct XenoActor *actor;        /* +0x00 */
    unsigned char unmodeled_04[0x870 - 4];
} UwamonoSlot;

extern UwamonoSlot uwamonoSlotTable[];

/* One 0x870-byte record of uwamonoRecords (tu051.h, tu067.c), with only the fields
   touched by the robo-mode and state-change natives modeled. */
typedef struct UwamonoRecord {
    unsigned char unmodeled_0[2];
    unsigned char kind;             /* +0x2: MAP_KIND_* */
    unsigned char unmodeled_3[0x24C - 3];
    short eventTimer;               /* +0x24C */
    int event;                      /* +0x250: 0 state change, 1 player model change, -1 none */
    int eventState;                 /* +0x254 */
    int eventStateArg;              /* +0x258 */
    unsigned char unmodeled_25c[0x330 - 0x25C];
    struct XenoActor *actor;        /* +0x330 */
    unsigned char unmodeled_334[0x338 - 0x334];
    float form;                     /* +0x338: 1.0 in the ES form (robo 0), 5.0 otherwise */
    unsigned char unmodeled_33c[0x870 - 0x33C];
} UwamonoRecord;

/* UwamonoRecord.kind (tu050.c's MapObjectRecord.kind). */
enum {
    MAP_KIND_FREE = 0,
    MAP_KIND_ACTOR = 1,
    MAP_KIND_MODEL = 2,
    MAP_KIND_FLAG = 3,
    MAP_KIND_ACTOR_STATIC = 4,  /* an actor that is not moved by uwamonoUpdateRecord */
    MAP_KIND_PART = 5,          /* a part attached to the player (tu051.c) */
    MAP_KIND_EFFECT = 6,        /* the record carries a sef effect at +0x638 */
    MAP_KIND_MOVER = 7,
    MAP_KIND_MODEL_REQUEST = 8  /* create only: uwamonoCreateRecord turns it into a model or frees it */
};

#define MAP_RECORD_COUNT 0x100
#define NULL ((void *) 0)

extern UwamonoRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;
extern short uwamonoLadderState;
void uwamonoSetMotion(int index, int motion, float speed);

/* Native "progEffect": argument 2 is the frame count.  Without argument 1 the first
   argument of sefExecEffectFrames is 0; with it, argument 2 is also read as a key
   and the ProgEffectEntry found by uwamonoFindIndexByShortKey supplies the effect. */
int runtimeProgEffect(int call)
{
    int frames;
    int lookupKey;
    int lookupIndex;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    frames = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 1) != 0) {
        sefExecEffectFrames(0, (float) frames);
    } else {
        lookupKey = func_A9F0C0(call, 2);
        lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
        sefExecEffectFrames((int) progEffectTable[lookupIndex].effect, (float) frames);
    }
    return 1;
}

int changeRobo(int call)
{
    int id;

    if (func_A9F0A8(call) != 1) {
        func_A9CC70(call, 2);
        return 0;
    }
    id = func_A9F0C0(call, 1);
    uwamonoSetRoboMode(id);
    return 1;
}

/* Called by changeRobo with the robo id.  Works on uwamonoRecords[uwamonoCurrentIndex]:
   returns early when the record's form already matches the id (1.0 for 0, 5.0 for 1);
   otherwise it starts a player model change (event 1) with timer 30, with no state
   (eventState/eventStateArg -1), sets CONTROL_SCRIPTED and calls fadeStartOut(0). */
void uwamonoSetRoboMode(signed char id)
{
    UwamonoRecord *record = &uwamonoRecords[uwamonoCurrentIndex];

    if (record->form == 1.0f) {
        if (id == 0) {
            return;
        }
    }
    if (record->form == 5.0f) {
        if (id == 1) {
            return;
        }
    }
    record->eventState = -1;
    record->eventTimer = 30;
    gameWork.control |= CONTROL_SCRIPTED;
    record->event = 1;
    record->eventStateArg = -1;
    fadeStartOut(0);
}

/* Native "setElevator": argument 1 is a record key, arguments 2 and 3 are the
   elevator motion and its argument stored in that record. */
int runtimeSetElevator(int call)
{
    int key;
    int index;
    IndexedRecord *record;
    int elevatorMotion;
    int elevatorArg;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    elevatorMotion = func_A9F0C0(call, 2);
    elevatorArg = func_A9F0C0(call, 3);
    record = &D_00340350[index];
    record->elevatorMotion = elevatorMotion;
    record->elevatorArg = elevatorArg;
    return 1;
}

/* Native "message": argument 1 is an index into D_003E7AF8 and argument 2 the int stored there. */
int runtimeMessage(int call)
{
    int index;
    int value;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    index = func_A9F0C0(call, 1);
    value = func_A9F0C0(call, 2);
    D_003E7AF8[index] = value;
    return 1;
}

/* Native "mapChange": argument 1 is the id that mainSwitchId makes the current one, with mode 1. */
int runtimeMapChange(int call)
{
    int id;

    if (func_A9F0A8(call) != 1) {
        func_A9CC70(call, 2);
        return 0;
    }
    id = func_A9F0C0(call, 1);
    mainSwitchId(id, 1);
    return 1;
}

/* Native "Fade": arguments 1 to 4 are red, green, blue and alpha, argument 5 the frame count,
   in the order fadeStartColorAlpha takes them. */
int runtimeFade(int call)
{
    int frames;
    int red;
    int green;
    int blue;
    int alpha;

    if (func_A9F0A8(call) != 5) {
        func_A9CC70(call, 2);
        return 0;
    }
    red = func_A9F0C0(call, 1);
    green = func_A9F0C0(call, 2);
    blue = func_A9F0C0(call, 3);
    alpha = func_A9F0C0(call, 4);
    frames = func_A9F0C0(call, 5);
    fadeStartColorAlpha(frames, (float) red, (float) green, (float) blue, (float) alpha);
    return 1;
}

/* Native "jumpCF": arguments 1 and 2 are the state and its argument (default -1) for a
   deferred state change, argument 3 a mode (default 1) and argument 4 the timer
   (default 30), passed on to uwamonoSetTransition.  Does nothing while a state change
   is already pending on the current record. */
int runtimeJumpCf(int call)
{
    UwamonoRecord *record = &uwamonoRecords[uwamonoCurrentIndex];
    int state;
    int stateArg;
    signed char mode;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (record->eventTimer != -1 && record->event == 0) {
        return 1;
    }
    state = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        stateArg = func_A9F0C0(call, 2);
    } else {
        stateArg = -1;
    }
    if (func_A9F1C0(call, 3) == 0) {
        mode = (signed char) func_A9F0C0(call, 3);
    } else {
        mode = 1;
    }
    uwamonoSetTransition(mode, state, stateArg,
        func_A9F1C0(call, 4) != 0 ? 30 : func_A9F0C0(call, 4));
    return 1;
}

/* Called by runtimeJumpCf (jumpCF) with a mode (default 1), a state and its argument
   and a timer (default 30) from the script.  Queues a deferred state change (event 0)
   on the current uwamono record: eventState, eventStateArg and eventTimer take the
   values.  Sets gameWork.control bit 0x10000 for mode 1; otherwise clears it and,
   while uwamonoLadderState is 0, starts motion 0 on the current record.  When
   gameWork.control bit 0x100000 is clear and timer is nonzero, fades out over timer
   frames.  When timer is zero the state change runs at once: the event becomes -1
   (none) and gameWork.state/stateArg take the record's values. */
void uwamonoSetTransition(signed char mode, int state, int stateArg, int timer)
{
    UwamonoRecord *record = &uwamonoRecords[uwamonoCurrentIndex];

    gameWork.control |= 0x20000000 | CONTROL_SCRIPTED;
    record->eventState = state;
    record->eventStateArg = stateArg;
    record->eventTimer = timer;
    record->event = 0;

    if (mode == 1) {
        gameWork.control |= 0x10000;
    } else {
        gameWork.control &= ~0x10000;
        if (uwamonoLadderState == 0) {
            uwamonoSetMotion(uwamonoCurrentIndex, 0, 1.0f);
        }
    }

    if (!(gameWork.control & 0x100000)) {
        if (timer != 0) {
            fadeStartOut(timer);
        }
    }
    if (timer == 0) {
        record->event = -1;
        gameWork.state = record->eventState;
        gameWork.stateArg = record->eventStateArg;
    }
}

/* Native "setFlag": argument 1 is the bit offset, argument 2 the bit count
   (1 when absent) and argument 3 the value, as the xglFlagsSet parameters. */
int runtimeSetFlag(int call)
{
    int bitCount;
    int bitOffset;
    int flagValue;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    bitCount = 1;
    bitOffset = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        bitCount = func_A9F0C0(call, 2);
    }
    flagValue = func_A9F0C0(call, 3);
    xglFlagsSet(bitOffset, bitCount, flagValue);
    return 1;
}

/* Native "getFlag": argument 1 is the bit offset, argument 2 the bit count
   (1 when absent), as the xglFlagsGet parameters.  Returns the flag value,
   not 1. */
int runtimeGetFlag(int call)
{
    int bitOffset;
    int bitCount;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    bitOffset = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) != 0) {
        bitCount = 1;
    } else {
        bitCount = func_A9F0C0(call, 2);
    }
    return xglFlagsGet(bitOffset, bitCount);
}

/* Native "setFog": argument 1 is a uwamono key, -1 or -2 for every slot (-1 also stores
   the fog in gameWork); arguments 2-5 are the fog distances in hundredths (near, far,
   min, max for pxySetFogDist) and 6-8 the fog color, written to each slot's actor,
   whose drawFlags gets XENO_DRAW_COLOR (0x1, xenoActor.c).  A single key sets no
   actor: the range ends at the found index itself (end = first). */
int runtimeSetMapFog(int call)
{
    int key;
    int first;
    int end;
    int index;
    /* Declared live before the loop and assigned after the slot test: the
       original loads the slot's actor into $2 for the test and copies it to
       $16 for the stores (one variable assigned in the test loads $16 directly). */
    struct XenoActor *actor = NULL;

    if (func_A9F0A8(call) != 8) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    if (key == -1) {
        gameWork.fogDist[0] = (float) func_A9F0C0(call, 2) / 100.0f;
        gameWork.fogDist[1] = (float) func_A9F0C0(call, 3) / 100.0f;
        gameWork.fogDist[2] = (float) func_A9F0C0(call, 4) / 100.0f;
        gameWork.fogDist[3] = (float) func_A9F0C0(call, 5) / 100.0f;
        gameWork.fogColor[0] = (float) func_A9F0C0(call, 6);
        gameWork.fogColor[1] = (float) func_A9F0C0(call, 7);
        gameWork.fogColor[2] = (float) func_A9F0C0(call, 8);
    }
    if (key == -1 || key == -2) {
        first = 0;
        end = MAP_RECORD_COUNT;
    } else {
        /* The original ends the range at the found index itself (end = first, not
           first + 1 as in setItai), so a single key sets no slot's fog; kept as compiled. */
        first = uwamonoFindIndexByShortKey((short) key);
        end = first;
    }
    for (index = first; index < end; index++) {
        if (uwamonoSlotTable[index].actor != NULL) {
            actor = uwamonoSlotTable[index].actor;
            actor->fogDist[0] = (float) func_A9F0C0(call, 2) / 100.0f;
            actor->fogDist[1] = (float) func_A9F0C0(call, 3) / 100.0f;
            actor->fogDist[2] = (float) func_A9F0C0(call, 4) / 100.0f;
            actor->fogDist[3] = (float) func_A9F0C0(call, 5) / 100.0f;
            actor->fogColor[0] = (float) func_A9F0C0(call, 6);
            actor->fogColor[1] = (float) func_A9F0C0(call, 7);
            actor->fogColor[2] = (float) func_A9F0C0(call, 8);
            actor->drawFlags |= XENO_DRAW_COLOR;
        }
    }
    return 1;
}

int setMapColor(int call)
{
    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    gameWork.color[0] = (float) func_A9F0C0(call, 1) / 255.0f;
    gameWork.color[1] = (float) func_A9F0C0(call, 2) / 255.0f;
    gameWork.color[2] = (float) func_A9F0C0(call, 3) / 255.0f;
    return 1;
}

/* Native "setItai", up to 10 arguments: argument 2 (default 1) goes to gameWork.shadowCasterMode;
   argument 3 is a uwamono key (every record when absent or negative).  For each live
   record with an actor, arguments 4/5 set/clear drawFlags bits, 6/7 set/clear
   renderFlags bits, 8 is a percentage for modeValue4 and 9 fills the first free (0xFF)
   colorIndex entry.  Argument 10 sets or clears gameWork.control bit 0x200000. */
int runtimeSetItai(int call)
{
    int first;
    int end;
    int index;
    int entry;

    if (func_A9F0A8(call) > 10) {
        func_A9CC70(call, 2);
        return 0;
    }
    gameWork.shadowCasterMode = func_A9F1C0(call, 2) != 0 ? 1 : func_A9F0C0(call, 2);
    first = 0;
    end = MAP_RECORD_COUNT;
    if (func_A9F1C0(call, 3) == 0 && func_A9F0C0(call, 3) >= 0) {
        first = uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 3));
        end = first + 1;
    }
    for (index = first; index < end; index++) {
        if (uwamonoRecords[index].kind != MAP_KIND_FREE && uwamonoRecords[index].actor != NULL) {
            if (func_A9F1C0(call, 4) == 0) {
                uwamonoRecords[index].actor->drawFlags |= func_A9F0C0(call, 4);
            }
            if (func_A9F1C0(call, 5) == 0) {
                uwamonoRecords[index].actor->drawFlags &= ~func_A9F0C0(call, 5);
            }
            if (func_A9F1C0(call, 6) == 0) {
                uwamonoRecords[index].actor->renderFlags |= func_A9F0C0(call, 6);
            }
            if (func_A9F1C0(call, 7) == 0) {
                uwamonoRecords[index].actor->renderFlags &= ~func_A9F0C0(call, 7);
            }
            if (func_A9F1C0(call, 8) == 0) {
                uwamonoRecords[index].actor->modeValue4 = (float) func_A9F0C0(call, 8) / 100.0f;
            }
            if (func_A9F1C0(call, 9) == 0) {
                for (entry = 0; entry < 16; entry++) {
                    if (uwamonoRecords[index].actor->colorIndex[entry] == 0xFF) {
                        uwamonoRecords[index].actor->colorIndex[entry] = func_A9F0C0(call, 9);
                        break;
                    }
                }
            }
        }
    }
    if (func_A9F1C0(call, 10) == 0) {
        if (func_A9F0C0(call, 10) != 0) {
            gameWork.control |= 0x200000;
        } else {
            gameWork.control &= ~0x200000;
        }
    }
    return 1;
}

/* Native "setLight", 8 arguments: argument 1 selects one of 16 light sets in gameWork,
   argument 2 the light type (0 ambient color, 1-3 a direction and color, 4 a direction
   and a value), arguments 3-5 a color out of 256 and 6-8 a direction out of 100.  A
   lone argument 3 gives a gray light color; type 4 without 6-8 disables the direction
   (w 0) and without 3-5 takes the value 0.2. */
int runtimeSetMapLight(int call)
{
    Vector4 direction;
    Vector4 color;
    int light;
    int type;

    if (func_A9F0A8(call) != 8) {
        func_A9CC70(call, 2);
        return 0;
    }
    light = func_A9F0C0(call, 1);
    type = func_A9F0C0(call, 2);
    color.x = (float) func_A9F0C0(call, 3) / 256.0f;
    color.y = (float) func_A9F0C0(call, 4) / 256.0f;
    color.z = (float) func_A9F0C0(call, 5) / 256.0f;
    direction.x = (float) func_A9F0C0(call, 6) / 100.0f;
    direction.y = (float) func_A9F0C0(call, 7) / 100.0f;
    direction.z = (float) func_A9F0C0(call, 8) / 100.0f;
    switch (type) {
        case 0:
            gameWork.ambientColor[light][0] = color.x;
            gameWork.ambientColor[light][1] = color.y;
            gameWork.ambientColor[light][2] = color.z;
            break;
        case 1:
        case 2:
        case 3:
            xglVectorNormal_001A9200(&direction, &direction);
            gameWork.lightDirection[light][type - 1][0] = direction.x;
            gameWork.lightDirection[light][type - 1][1] = direction.y;
            gameWork.lightDirection[light][type - 1][2] = direction.z;
            gameWork.lightDirection[light][type - 1][3] = 1.0f;
            if (func_A9F1C0(call, 3) == 0 && func_A9F1C0(call, 4) != 0 && func_A9F1C0(call, 5) != 0) {
                gameWork.lightColor[light][type - 1][0] = color.x;
                gameWork.lightColor[light][type - 1][1] = color.x;
                gameWork.lightColor[light][type - 1][2] = color.x;
                gameWork.lightColor[light][type - 1][3] = 1.0f;
            } else {
                gameWork.lightColor[light][type - 1][0] = color.x;
                gameWork.lightColor[light][type - 1][1] = color.y;
                gameWork.lightColor[light][type - 1][2] = color.z;
                gameWork.lightColor[light][type - 1][3] = 1.0f;
            }
            break;
        case 4:
            if (func_A9F1C0(call, 6) != 0 && func_A9F1C0(call, 7) != 0 && func_A9F1C0(call, 8) != 0) {
                gameWork.type4Direction[light][3] = 0.0f;
            } else {
                xglVectorNormal_001A9200(&direction, &direction);
                gameWork.type4Direction[light][0] = direction.x;
                gameWork.type4Direction[light][1] = direction.y;
                gameWork.type4Direction[light][2] = direction.z;
                gameWork.type4Direction[light][3] = 1.0f;
            }
            if (func_A9F1C0(call, 3) != 0 && func_A9F1C0(call, 4) != 0 && func_A9F1C0(call, 5) != 0) {
                gameWork.type4Value[light] = 0.2f;
            } else {
                gameWork.type4Value[light] = color.x;
            }
            break;
    }
    return 1;
}

/* Native "System": argument 1 selects one of several gameWork.control bits to set or
   clear; out of range (or missing) values are ignored.  Always returns 1 once the
   argument count check passes. */
int runtimeSystem(int call)
{
    int flagId;

    if (func_A9F0A8(call) <= 0) {
        func_A9CC70(call, 2);
        return 0;
    }
    flagId = func_A9F0C0(call, 1);
    switch (flagId) {
        case 0:
            gameWork.control |= 0x08000000;
            break;
        case 1:
            gameWork.control |= 0x00080000;
            break;
        case 2:
            gameWork.control &= ~0x00080000;
            break;
        case 3:
            break;
        case 4:
            gameWork.control |= 0x00800000;
            break;
        case 5:
            gameWork.control |= 0x20000000;
            break;
        case 6:
            gameWork.control &= ~0x20000000;
            break;
        case 7:
            gameWork.control |= 0x40000000;
            break;
        case 8:
            gameWork.control &= ~0x40000000;
            break;
        case 9:
            gameWork.control &= ~0x00100000;
            break;
        case 10:
            gameWork.control |= 0x00100000;
            break;
    }
    return 1;
}
