#include "common.h"
#include "shared.h"
#include "main/tu050.h"
#include "main/tu165.h"
#include "main/xgl_flags.h"

/* Script native argument access: the VM supplies the argument count and the
   integer argument at each 1-based position of the running native call. */
int func_A9F0A8(void);
void func_A9CC70(int call, int error_code);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);
int uwamonoFindIndexByShortKey(short key);
void uwamonoSetRoboMode(signed char id);
void fadeStartColorAlpha(int frames, float red, float green, float blue, float alpha);
int xglFlagsGet(int bit_offset, int bit_count);
void fadeStartOut(int frames);
void uwamonoSetTransition(signed char mode, int valueA, int valueB, int time);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

/* Records of 0x870 bytes indexed by uwamonoFindIndexByShortKey.  Only the touched
   fields are modeled; the gaps are left unmodeled. */
typedef struct IndexedRecord {
    unsigned char unmodeled_0[2];
    short short_value;
    int word_value;
    unsigned char unmodeled_8[0x870 - 8];
} IndexedRecord;

/* The same 0x870-byte stride, read by progEffect: the first word of each element
   is the effect id passed to sefExecEffectFrames. */
typedef struct ProgEffectEntry {
    int effect_id;
    unsigned char unmodeled_4[0x870 - 4];
} ProgEffectEntry;

extern IndexedRecord D_00340350[];
extern ProgEffectEntry progEffectTable[];
extern int D_003E7AF8[];

typedef struct MapColorBlock {
    unsigned char unmodeled_0[8];
    int control;                    /* +0x8: bits 0x8000/0x10000/0x100000/0x20000000 scripted-transition state */
    unsigned char unmodeled_c[0x14 - 0xC];
    int transitionValueA;           /* +0x14: mirrors uwamono record transitionValueA while transitioning */
    unsigned char unmodeled_18[0x1C - 0x18];
    int transitionValueB;           /* +0x1C: mirrors uwamono record transitionValueB while transitioning */
    unsigned char unmodeled_20[0xC0 - 0x20];
    float color[3];
    unsigned char unmodeled_cc[0x348 - 0xCC];
    float actorColor[4];            /* +0x348: setEffectColor's percentages / 100 for every actor */
    unsigned char unmodeled_358[0x360 - 0x358];
    float actorColorBlock[3];       /* +0x360: setEffectColor's block values for every actor */
    unsigned char unmodeled_36c[0x370 - 0x36C];
    int actorParam;                 /* +0x370 */
    unsigned char unmodeled_374[0x384 - 0x374];
    float lightDirection[16][3][4]; /* +0x384: per light set, light types 1-3 */
    float lightColor[16][3][4];     /* +0x684 */
    float ambientColor[16][4];      /* +0x984: light type 0 */
    float type4Direction[16][4];    /* +0xA84: light type 4; w 0 disables it */
    float type4Value[16];           /* +0xB84: light type 4, 0.2 by default */
} MapColorBlock;

extern MapColorBlock gameWork;

/* The XenoActor fields (xenoActor.c names) the color natives write. */
struct XenoActor {
    unsigned char unmodeled_000[0xBC];
    u32 drawFlags;                  /* 0x0BC */
    unsigned char unmodeled_0c0[0xD0 - 0xC0];
    int renderFlags;                /* 0x0D0 */
    unsigned char unmodeled_0d4[0x180 - 0xD4];
    float modeValue4;               /* 0x180 */
    unsigned char unmodeled_184[0x490 - 0x184];
    float color[4];                 /* 0x490: setEffectColor's percentages / 100 */
    float colorBlock[3];            /* 0x4A0: setEffectColor's block values */
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

/* The same 0x870-byte stride as uwamonoRecords (tu051.h), with only the fields
   touched by the robo-mode/transition natives modeled. */
typedef struct UwamonoTransitionRecord {
    unsigned char unmodeled_0[2];
    unsigned char kind;             /* +0x2: nonzero for a live record */
    unsigned char unmodeled_3[0x24C - 3];
    short transitionTime;           /* +0x24C */
    int transitionActive;           /* +0x250 */
    int transitionValueA;           /* +0x254 */
    int transitionValueB;           /* +0x258 */
    unsigned char unmodeled_25c[0x330 - 0x25C];
    struct XenoActor *actor;        /* +0x330 */
    unsigned char unmodeled_334[0x338 - 0x334];
    float mode;                     /* +0x338: 1.0 for robo mode 0, 5.0 for robo mode 1 */
    unsigned char unmodeled_33c[0x870 - 0x33C];
} UwamonoTransitionRecord;

extern UwamonoTransitionRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;
extern short uwamonoLadderState;
void uwamonoSetMotion(int index, int motion, float speed);

/* Native "progEffect": argument 2 is the frame count.  Without argument 1 the first
   argument of sefExecEffectFrames is 0; with it, argument 2 is also read as a key
   and the ProgEffectEntry found by uwamonoFindIndexByShortKey supplies the effect id. */
int runtimeProgEffect(int call)
{
    int frames;
    int lookupKey;
    int lookupIndex;

    if (func_A9F0A8() < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    frames = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 1) != 0) {
        sefExecEffectFrames(0, (float) frames);
    } else {
        lookupKey = func_A9F0C0(call, 2);
        lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
        sefExecEffectFrames(progEffectTable[lookupIndex].effect_id, (float) frames);
    }
    return 1;
}

int changeRobo(int call)
{
    int id;

    if (func_A9F0A8() != 1) {
        func_A9CC70(call, 2);
        return 0;
    }
    id = func_A9F0C0(call, 1);
    uwamonoSetRoboMode(id);
    return 1;
}

/* Called by changeRobo with the robo id.  Works on uwamonoRecords[uwamonoCurrentIndex]:
   returns early when the record's mode float (+0x338) already equals the value for
   the id (1.0 for 0, 5.0 for 1); otherwise it starts a change with time 30. */
void uwamonoSetRoboMode(signed char id)
{
    UwamonoTransitionRecord *record = &uwamonoRecords[uwamonoCurrentIndex];

    if (record->mode == 1.0f) {
        if (id == 0) {
            return;
        }
    }
    if (record->mode == 5.0f) {
        if (id == 1) {
            return;
        }
    }
    record->transitionValueA = -1;
    record->transitionTime = 30;
    gameWork.control |= 0x8000;
    record->transitionActive = 1;
    record->transitionValueB = -1;
    fadeStartOut(0);
}

/* Native "setElevator": argument 1 is a lookup key, arguments 2 and 3 are
   stored in the IndexedRecord that the key selects. */
int runtimeSetElevator(int call)
{
    int lookupKey;
    int lookupIndex;
    IndexedRecord *record;
    int shortValue;
    int wordValue;

    if (func_A9F0A8() != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    lookupKey = func_A9F0C0(call, 1);
    lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
    shortValue = func_A9F0C0(call, 2);
    wordValue = func_A9F0C0(call, 3);
    record = &D_00340350[lookupIndex];
    record->short_value = shortValue;
    record->word_value = wordValue;
    return 1;
}

/* Native "message": argument 1 is an index into D_003E7AF8 and argument 2 the int stored there. */
int runtimeMessage(int call)
{
    int index;
    int value;

    if (func_A9F0A8() != 2) {
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

    if (func_A9F0A8() != 1) {
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

    if (func_A9F0A8() != 5) {
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

/* Native "jumpCF": arguments 1 and 2 are two int values for the transition, argument 3
   a mode (default 1) and argument 4 a short time (default 30), passed on to
   uwamonoSetTransition.  Does nothing when the current record already has a
   transition time set and is not active. */
int runtime_001F3920(int call)
{
    UwamonoTransitionRecord *record = &uwamonoRecords[uwamonoCurrentIndex];
    int valueA;
    int valueB;
    signed char mode;

    if (func_A9F0A8() < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (record->transitionTime != -1 && record->transitionActive == 0) {
        return 1;
    }
    valueA = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        valueB = func_A9F0C0(call, 2);
    } else {
        valueB = -1;
    }
    if (func_A9F1C0(call, 3) == 0) {
        mode = (signed char) func_A9F0C0(call, 3);
    } else {
        mode = 1;
    }
    uwamonoSetTransition(mode, valueA, valueB,
        func_A9F1C0(call, 4) != 0 ? 30 : func_A9F0C0(call, 4));
    return 1;
}

/* Called by runtime_001F3920 (jumpCF) with a mode (default 1), two int values and a
   time (default 30) from the script.  Stores them in the current uwamono record at
   +0x254, +0x258 and +0x24C and clears +0x250.  Sets gameWork.control bit 0x10000 for
   mode 1; otherwise clears it and, while uwamonoLadderState is 0, starts motion 0 on
   the current record.  When gameWork.control bit 0x100000 is clear and time is
   nonzero, fades out over time frames.  When time is zero, marks the record inactive
   (-1) and mirrors its two stored values into gameWork. */
void uwamonoSetTransition(signed char mode, int valueA, int valueB, int time)
{
    UwamonoTransitionRecord *record = &uwamonoRecords[uwamonoCurrentIndex];

    gameWork.control |= 0x20008000;
    record->transitionValueA = valueA;
    record->transitionValueB = valueB;
    record->transitionTime = time;
    record->transitionActive = 0;

    if (mode == 1) {
        gameWork.control |= 0x10000;
    } else {
        gameWork.control &= ~0x10000;
        if (uwamonoLadderState == 0) {
            uwamonoSetMotion(uwamonoCurrentIndex, 0, 1.0f);
        }
    }

    if (!(gameWork.control & 0x100000)) {
        if (time != 0) {
            fadeStartOut(time);
        }
    }
    if (time == 0) {
        record->transitionActive = -1;
        gameWork.transitionValueA = record->transitionValueA;
        gameWork.transitionValueB = record->transitionValueB;
    }
}

/* Native "setFlag": argument 1 is the bit offset, argument 2 the bit count
   (1 when absent) and argument 3 the value, as the xglFlagsSet parameters. */
int runtimeSetFlag(int call)
{
    int bitCount;
    int bitOffset;
    int flagValue;

    if (func_A9F0A8() != 3) {
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
int runtime_001F3C20(int call)
{
    int bitOffset;
    int bitCount;

    if (func_A9F0A8() != 2) {
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

/* Native "setEffectColor": argument 1 is a uwamono key, -1 or -2 for every slot (-1
   also stores the color in gameWork); arguments 2-5 are color percentages and 6-8
   the colorBlock values written to each slot's actor, which gets drawFlags bit 1. */
int runtime_001F3CB8(int call)
{
    int key;
    int first;
    int end;
    int index;
    /* Declared live before the loop and assigned after the slot test: the
       original loads the slot's actor into $2 for the test and copies it to
       $16 for the stores (one variable assigned in the test loads $16 directly). */
    struct XenoActor *actor = 0;

    if (func_A9F0A8() != 8) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    if (key == -1) {
        gameWork.actorColor[0] = (float) func_A9F0C0(call, 2) / 100.0f;
        gameWork.actorColor[1] = (float) func_A9F0C0(call, 3) / 100.0f;
        gameWork.actorColor[2] = (float) func_A9F0C0(call, 4) / 100.0f;
        gameWork.actorColor[3] = (float) func_A9F0C0(call, 5) / 100.0f;
        gameWork.actorColorBlock[0] = (float) func_A9F0C0(call, 6);
        gameWork.actorColorBlock[1] = (float) func_A9F0C0(call, 7);
        gameWork.actorColorBlock[2] = (float) func_A9F0C0(call, 8);
    }
    if (key == -1 || key == -2) {
        first = 0;
        end = 0x100;
    } else {
        /* The original ends the range at the found index itself, so a single
           key colors no slot; kept as compiled. */
        first = uwamonoFindIndexByShortKey(key);
        end = first;
    }
    for (index = first; index < end; index++) {
        if (uwamonoSlotTable[index].actor != 0) {
            actor = uwamonoSlotTable[index].actor;
            actor->color[0] = (float) func_A9F0C0(call, 2) / 100.0f;
            actor->color[1] = (float) func_A9F0C0(call, 3) / 100.0f;
            actor->color[2] = (float) func_A9F0C0(call, 4) / 100.0f;
            actor->color[3] = (float) func_A9F0C0(call, 5) / 100.0f;
            actor->colorBlock[0] = (float) func_A9F0C0(call, 6);
            actor->colorBlock[1] = (float) func_A9F0C0(call, 7);
            actor->colorBlock[2] = (float) func_A9F0C0(call, 8);
            actor->drawFlags |= 1;
        }
    }
    return 1;
}

int setMapColor(int call)
{
    if (func_A9F0A8() != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    gameWork.color[0] = (float) func_A9F0C0(call, 1) / 255.0f;
    gameWork.color[1] = (float) func_A9F0C0(call, 2) / 255.0f;
    gameWork.color[2] = (float) func_A9F0C0(call, 3) / 255.0f;
    return 1;
}

/* Native with up to 10 arguments: argument 2 (default 1) goes to gameWork.actorParam;
   argument 3 is a uwamono key (every record when absent or negative).  For each live
   record with an actor, arguments 4/5 set/clear drawFlags bits, 6/7 set/clear
   renderFlags bits, 8 is a percentage for modeValue4 and 9 fills the first free (0xFF)
   colorIndex entry.  Argument 10 sets or clears gameWork.control bit 0x200000. */
int runtime_001F40A0(int call)
{
    int first;
    int end;
    int index;
    int entry;

    if (func_A9F0A8() > 10) {
        func_A9CC70(call, 2);
        return 0;
    }
    gameWork.actorParam = func_A9F1C0(call, 2) != 0 ? 1 : func_A9F0C0(call, 2);
    first = 0;
    end = 0x100;
    if (func_A9F1C0(call, 3) == 0 && func_A9F0C0(call, 3) >= 0) {
        first = uwamonoFindIndexByShortKey(func_A9F0C0(call, 3));
        end = first + 1;
    }
    for (index = first; index < end; index++) {
        if (uwamonoRecords[index].kind != 0 && uwamonoRecords[index].actor != 0) {
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

/* Light native with 8 arguments: argument 1 selects one of 16 light sets in gameWork,
   argument 2 the light type (0 ambient color, 1-3 a direction and color, 4 a direction
   and a value), arguments 3-5 a color out of 256 and 6-8 a direction out of 100.  A
   lone argument 3 gives a gray light color; type 4 without 6-8 disables the direction
   (w 0) and without 3-5 takes the value 0.2. */
int runtime_001F43D0(int call)
{
    Vector4 direction;
    Vector4 color;
    int light;
    int type;

    if (func_A9F0A8() != 8) {
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

/* Native "setFlags": argument 1 selects one of several gameWork.control bits to set or
   clear; out of range (or missing) values are ignored.  Always returns 1 once the
   argument count check passes. */
int runtime_001F47C8(int call)
{
    int flagId;

    if (func_A9F0A8() <= 0) {
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
