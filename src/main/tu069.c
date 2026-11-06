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
    unsigned char unmodeled_0[0xC0];
    float color[3];
} MapColorBlock;

extern MapColorBlock gameWork;

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
INCLUDE_ASM("asm/main/nonmatchings/tu069", uwamonoSetRoboMode);

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

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F3920);

/* Called by runtime_001F3920 (jumpCF) with a mode (default 1), two int values and a
   short time (default 30) from the script.  Stores them in the current uwamono record
   at +0x254, +0x258 and +0x24C, clears +0x250 and sets or clears bit 0x10000 of the
   word at gameWork+8 according to the mode. */
INCLUDE_ASM("asm/main/nonmatchings/tu069", uwamonoSetTransition);

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

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F3C20);

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F3CB8);

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

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F40A0);

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F43D0);

INCLUDE_ASM("asm/main/nonmatchings/tu069", runtime_001F47C8);
