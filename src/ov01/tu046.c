/*
 * OV01 original TU 46: 0x00a9f0a8..0x00a9ff80 (35 functions)
 */
#include "common.h"
#include "main/tu173.h"
#include "ov01/tu022.h"
#include "ov01/tu031.h"
#include "ov01/tu041.h"
#include "ov01/tu044.h"
#include "ov01/tu046.h"
#include "ov01/tu070.h"

/*
 * Script VM context a plugin call receives. The VM keeps its operand stack as
 * 4-byte words: stackCount words are in use, and the word just below the top
 * is the current frame's argument count. Each argument owns an 8-byte slot
 * that ends below that count word, its low word first.
 */
typedef struct VmPluginContext {
    unsigned char unmodeled_00[0x04];
    int stackCount;   /* +0x04: words in use on the operand stack */
    unsigned char unmodeled_08[0x14 - 0x08];
    struct VmPluginHeader *header; /* +0x14 */
    unsigned char unmodeled_18[0x1C - 0x18];
    int *stack;       /* +0x1C: base of the operand stack */
} VmPluginContext;

typedef struct VmPluginHeader VmPluginHeader;

struct VmPluginHeader {
    unsigned char unmodeled_00[0x18];
    int entryOffset; /* +0x18: distance from this header to the entry array */
};

int VM_PLUGIN_CNT_GET(vm)
VmPluginContext *vm;
{
    return vm->stack[vm->stackCount - 1];
}

#include "shared.h"



/*
 * Argument slots step by 8 bytes below the count word. The tag of argument k
 * is the first byte of its high word, which lies VM_ARG_TAG_BASE bytes below
 * the stack top for the newest argument.
 */
enum {
    VM_ARG_SLOT_BYTES = 8,
    VM_ARG_TAG_BASE = -8
};

/*
 * Partial view of the object unitTblEntryGet returns (dereferenced here, layout
 * otherwise not evidenced in this TU). Only actorGet's field at +0x1D0 is read.
 * tu017 declares unitTblEntryGet as returning UnitRecord *; its layout ends at
 * +0x2c, so the +0x1D0 actor belongs to that record once it is extended.
 */
typedef struct UnitView {
    unsigned char unmodeled_000[0x1D0];
    void *actor;      /* +0x1D0: actorGet returns it */
} UnitView;

/* Callers pass no argument: the VM context is already in $a0 from the script dispatcher. */
int VM_PLUGIN_CNT_GET();
void sbFatal(VmPluginContext *vm);

/* Returns the low word of argument argIndex (1-based) of the current call. */
int vmArgGet(VmPluginContext *vm, int argIndex)
{
    int *frameTop;
    int argCount;

    frameTop = vm->stack + vm->stackCount;
    argCount = frameTop[-1];
    if (argCount < argIndex || argIndex == 0) {
        sbFatal(vm);
    }
    return frameTop[-3 - 2 * (argCount - argIndex)];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu046", vmArgGetHigh);

/*
 * Reports whether argument argIndex has a zero tag byte: the first byte of
 * its slot's high word. An index past the frame's argument count returns 1.
 */
int vmArgTagIsZero(VmPluginContext *vm, int argIndex)
{
    int *frameTop;
    int argCount;

    frameTop = vm->stack + vm->stackCount;
    if (argIndex == 0) {
        sbFatal(vm);
    }
    argCount = frameTop[-1];
    if (!(argCount >= argIndex)) {
        return 1;
    }
    return ((unsigned char *) frameTop)[VM_ARG_TAG_BASE - (argCount - argIndex) * VM_ARG_SLOT_BYTES] == 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu046", vmKeyedEntryGet);

/* Address of word-sized entry `index` of the header's entry array. */
int vmEntryGet(VmPluginContext *vm, int index)
{
    return (int) ((int *)((char *)vm->header + vm->header->entryOffset) + index);
}

/*
 * Entry `index` holds word offsets from the entry array; returns the entry
 * that word `subIndex` of it points to.
 */
int vmSubEntryGet(VmPluginContext *vm, int index, int subIndex)
{
    int *base = (int *)((char *)vm->header + vm->header->entryOffset);
    int *entry = base + index;

    return (int) (base + entry[subIndex]);
}

void func_00A9F2E0(void)
{
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu046", func_00A9F2E8);

int vmEntryGet(VmPluginContext *vm, int index);
int vmSubEntryGet(VmPluginContext *vm, int index, int subIndex);
/* Entry chosen by posDataSet; posDataGet returns it. */
extern int posDataEntry;

int posDataSet(VmPluginContext *vm)
{
    int entryIndex;

    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    entryIndex = vmArgGet(vm, 1);
    posDataEntry = vmSubEntryGet(vm, entryIndex, 0);
    return 1;
}

/* Returns the entry posDataSet stored. */
int posDataGet(void)
{
    return posDataEntry;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu046", monsTblSet);

/* Entry monsTblSet selects: the first one whose word is zero; monsTblGet returns it. */
extern int monsTblEntry;

/* Returns the monster table entry monsTblSet selected. */
int monsTblGet(void)
{
    return monsTblEntry;
}

void func_00A99DC8(void);

void monsSetNoGet(void)
{
    func_00A99DC8();
}

/* First argument of the last cameraSet call, stored as given. */
extern int D_00AE0C28;

int cameraSet(VmPluginContext *vm)
{
    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    D_00AE0C28 = vmArgGet(vm, 1);
    return 1;
}

/* Returns the value cameraSet stored. */
int cameraSettingGet(void)
{
    return D_00AE0C28;
}

UnitView *unitTblEntryGet(int unitIndex);
void thinkDispatch(UnitView *unit, int argTwo, int argThree, UnitView *unitFour);
void thinkSendEnd(UnitView *unit);

int thinkSet(VmPluginContext *vm)
{
    UnitView *unitOne;
    UnitView *unitFour;
    int argTwo;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 4) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unitOne = unitTblEntryGet(vmArgGet(vm, 1));
    unitFour = unitTblEntryGet(vmArgGet(vm, 4));
    argTwo = vmArgGet(vm, 2);
    argThree = vmArgGet(vm, 3);
    thinkDispatch(unitOne, argTwo, argThree, unitFour);
    thinkSendEnd(unitOne);
    return 1;
}

int thinkSet2(VmPluginContext *vm)
{
    UnitView *unitOne;
    UnitView *unitFour;
    int argTwo;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 4) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unitOne = unitTblEntryGet(vmArgGet(vm, 1));
    unitFour = unitTblEntryGet(vmArgGet(vm, 4));
    argTwo = vmArgGet(vm, 2);
    argThree = vmArgGet(vm, 3);
    thinkDispatch(unitOne, argTwo, argThree, unitFour);
    return 1;
}

int thinkSetEnd(VmPluginContext *vm)
{
    UnitView *unit;

    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    thinkSendEnd(unit);
    return 1;
}

int func_00A9BF08(UnitView *unit, int argTwoValue, int value);

int atkTbl(VmPluginContext *vm)
{
    UnitView *unit;
    int value;
    int entryIndex;
    int entry;

    if (VM_PLUGIN_CNT_GET() < 2) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    value = 0xA;
    if (vmArgTagIsZero(vm, 3) == 0) {
        value = vmArgGet(vm, 3);
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    entryIndex = vmArgGet(vm, 2);
    entry = vmEntryGet(vm, entryIndex);
    return func_00A9BF08(unit, entry, value);
}

int func_00A9C070(int argOne, int argTwoValue, int argThree);

int atkTblGet(VmPluginContext *vm)
{
    int argOne;
    int entryIndex;
    int entry;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 3) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    entryIndex = vmArgGet(vm, 2);
    entry = vmEntryGet(vm, entryIndex);
    argOne = vmArgGet(vm, 1);
    argThree = vmArgGet(vm, 3);
    return func_00A9C070(argOne, entry, argThree);
}

void *actorGet(VmPluginContext *vm)
{
    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    return unitTblEntryGet(vmArgGet(vm, 1))->actor;
}

int unitParaFieldGet(UnitView *unit, int argTwo);

int unitParaGet(VmPluginContext *vm)
{
    UnitView *unit;
    int argTwo;

    if (VM_PLUGIN_CNT_GET() < 2) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    argTwo = vmArgGet(vm, 2);
    return unitParaFieldGet(unit, argTwo);
}

void unitParaFieldSet(UnitView *unit, int argTwo, int argThree);

int unitParaSet(VmPluginContext *vm)
{
    UnitView *unit;
    int argTwo;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 3) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    argTwo = vmArgGet(vm, 2);
    argThree = vmArgGet(vm, 3);
    unitParaFieldSet(unit, argTwo, argThree);
    return 1;
}

int func_00260300(int effectKey, int argOne);

/* The effect key packs the group above bit 12 and the effect index below it. */
int loadEffect(VmPluginContext *vm)
{
    int effectGroup;
    int effectIndex;

    if (VM_PLUGIN_CNT_GET() < 2) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    effectGroup = vmArgGet(vm, 1);
    effectIndex = vmArgGet(vm, 2);
    func_00260300((effectGroup << 0xC) | effectIndex, effectGroup);
    return 1;
}


int loadWaitEffect(int unused, int *flag)
{
    if (loadWaitPending() != 0) {
        *flag = 1;
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu046", createEffect);

void effectSendValue(int argOne, int argTwo, Vector4 *position);

int createEffectPos(VmPluginContext *vm)
{
    Vector4 position;
    float zRaw;
    int positionKey;
    int argOne;
    int argTwo;

    if (VM_PLUGIN_CNT_GET() < 2) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    positionKey = vmArgGet(vm, 3);
    position.x = (float) effectPosGet(positionKey)[0] / 100.0f;
    position.y = (float) effectPosGet(positionKey)[1] / 100.0f;
    zRaw = (float) effectPosGet(positionKey)[2];
    position.w = 1.0f;
    position.z = zRaw / 100.0f;
    argOne = vmArgGet(vm, 1);
    argTwo = vmArgGet(vm, 2);
    effectSendValue(argOne, argTwo, &position);
    return 1;
}

int func_00A9C328(int argOne, int argTwo);

int tecParaGet(VmPluginContext *vm)
{
    int argOne;
    int argTwo;

    if (VM_PLUGIN_CNT_GET() < 2) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    argOne = vmArgGet(vm, 1);
    argTwo = vmArgGet(vm, 2);
    return func_00A9C328(argOne, argTwo);
}

int calcStatGet(UnitView *unit, int argTwo, int argThree);

int unitStatGet(VmPluginContext *vm)
{
    UnitView *unit;
    int argTwo;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 3) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    argTwo = vmArgGet(vm, 2);
    argThree = vmArgGet(vm, 3);
    return calcStatGet(unit, argTwo, argThree);
}

int calcStatSet(UnitView *unit, int argTwo, int argThree);

int unitStatSet(VmPluginContext *vm)
{
    UnitView *unit;
    int argTwo;
    int argThree;

    if (VM_PLUGIN_CNT_GET() < 3) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    argTwo = vmArgGet(vm, 2);
    argThree = vmArgGet(vm, 3);
    calcStatSet(unit, argTwo, argThree);
    return 1;
}

int calcBoostOk(UnitView *unit, int mode);
int func_00A954D8(UnitView *unit, int width, int height);

int counterBoost(VmPluginContext *vm)
{
    UnitView *unit;
    int applied;

    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    applied = calcBoostOk(unit, 0);
    if (applied != 0) {
        calcActStockSet(unit, 0);
        func_00A954D8(unit, 0x140, 7);
        seVoiceStartTagged(2, 1);
        applied = 1;
    }
    return applied;
}

int boost(VmPluginContext *vm)
{
    UnitView *unit;
    int applied;

    if (VM_PLUGIN_CNT_GET() <= 0) {
        sbChainSetValue(vm, 2);
        return 0;
    }
    unit = unitTblEntryGet(vmArgGet(vm, 1));
    applied = calcBoostOk(unit, 1);
    if (applied != 0) {
        calcActStockSet(unit, 1);
        func_00A954D8(unit, 0x140, 7);
        seVoiceStartTagged(2, 1);
        applied = 1;
    }
    return applied;
}


int VPadEnable(void)
{
    dataVPadModeSet(1);
    return 1;
}

int VPadDisable(void)
{
    dataVPadModeSet(0);
    return 1;
}
