/*
 * OV01 original TU 23: 0x00a8a8b8..0x00a8b930 (31 functions)
 */
#include "common.h"
#include "ov01/tu023.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleWorkTablesReset);

typedef struct BattleEntryOwner BattleEntryOwner;

typedef struct BattleEntry BattleEntry;

typedef struct BattleSlot BattleSlot;

typedef struct BattleLookupEntry BattleLookupEntry;

/*
 * The record the functions below receive. Only its entry pointer is evidenced
 * here; the rest of the record is not claimed.
 */
struct BattleEntryOwner {
    unsigned char unmodeled_00[0x144];
    BattleEntry *work; /* +0x144: the entry this holder works on */
};

/*
 * One 0x108-byte entry of the pools battleEntryPoolA and battleEntryPoolB. index (+0x62) is
 * its key, and 0 marks a free entry (battleEntryAcquire allocates by it). value and
 * flags (+0x5C, +0x60) are copied into the slot selected by index through
 * battleSlotCommit. The reset_ members are zeroed by battleEntryClearReset.
 */
struct BattleEntry {
    unsigned char unmodeled_00[0x5C];
    int value;            /* +0x5C */
    unsigned short flags; /* +0x60 */
    short index;          /* +0x62 */
    unsigned char unmodeled_64[0xDA - 0x64];
    unsigned char reset_block_da[8]; /* +0xDA */
    unsigned char reset_block_e2[8]; /* +0xE2 */
    unsigned char unmodeled_ea[0xEC - 0xEA];
    int reset_word_ec;    /* +0xEC */
};

/* One 0x108-byte entry of battleSlotTable, selected by battleSlotGet (slot index 1 to 15). */
struct BattleSlot {
    unsigned char unmodeled_00[0x5C];
    int value;            /* +0x5C */
    unsigned short flags; /* +0x60 */
    unsigned char unmodeled_62[0x108 - 0x62];
};

/* The entry func_00A9B490 returns; battleContextApplyLookup reads its s16 at +8. */
struct BattleLookupEntry {
    unsigned char unmodeled_00[8];
    short value; /* +0x08 */
};

extern BattleSlot battleSlotTable[];
extern void *memset(void *destination, int value, unsigned int count);

void battleEntryClearReset(BattleEntryOwner *owner)
{
    BattleEntry *work = owner->work;

    memset(work->reset_block_da, 0, 8);
    memset(work->reset_block_e2, 0, 8);
    work->reset_word_ec = 0;
}

/*
 * Context record for one slot binding (battleContextTable, 0x1F0 bytes each; battleContextBind
 * returns it). Only the state pointer at +0x144 is evidenced here.
 */
typedef struct BattleContext BattleContext;

typedef struct BattleState BattleState;

struct BattleContext {
    unsigned char unmodeled_000[0x144];
    BattleState *state; /* +0x144 */
};

/*
 * State record bound to a context (battleStateTable, 0x108 bytes each). Its key-like
 * halfword at +0x62 is cleared by battleStateClearStatus; the words at +0xF0 and +0xF4 are
 * cleared by battleStateClearWords.
 */
struct BattleState {
    unsigned char unmodeled_00[0x62];
    unsigned short status_word; /* +0x62, cleared by battleStateClearStatus */
    unsigned char unmodeled_64[0xF0 - 0x64];
    int reset_words[2]; /* +0xF0 and +0xF4, both cleared by battleStateClearWords */
};

void battleStateClearWords(BattleContext *context)
{
    BattleState *state = context->state;

    state->reset_words[0] = 0;
    state->reset_words[1] = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleMaskWordsStat4);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleMaskWordsStat6);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleEntryFindByKey);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleEntryAcquire);

int battleStateClearStatus(BattleContext *context)
{
    BattleState *state = context->state;

    if (state != 0) {
        state->status_word = 0;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleSlotExport);

BattleSlot *battleSlotGet(int index)
{
    if (!(index < 0x10)) {
        return 0;
    }
    return &battleSlotTable[index - 1];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleFlushActiveEntries);

void battleSlotCommit(BattleEntryOwner *owner)
{
    BattleEntry *work = owner->work;
    BattleSlot *slot = battleSlotGet(work->index);

    slot->value = work->value;
    slot->flags = work->flags;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleSlotsInit);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleEntryLoad);

void func_00A8B2C8(void)
{
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleEntryActivate);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleContextBind);

/*
 * battleContextBind stays scaffold asm. It binds slot `key` to the state record of
 * the context selected by its second argument (every caller here passes 0) and
 * returns that context.
 */
BattleContext *battleContextBind(int, int);

BattleState *battleContextStateGet(int key)
{
    return battleContextBind(key, 0)->state;
}

void func_00A8C330(int handle, short value);
BattleLookupEntry *func_00A9B490(int lookup_key);

void battleContextApplyLookup(int key, int lookup_key)
{
    int handle = battleContextBind(key, 0);
    BattleLookupEntry *entry = func_00A9B490(lookup_key);

    func_00A8C330(handle, entry->value);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleAttackResolve);

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", func_00A8B658);

/*
 * Slot wrappers. Each binds the slot `key` to its context, then applies one slot
 * operation to it. Proposed names are in .work/x2-names/proposals/ov01-tu023.json.
 */

int slotFlagSet(int handle, int value);

/* battleSlotFlagSet: sets flag `flag` (1 to 64) of the slot's flag set. unitLearnMaskSet calls it for each set bit of a mask. */
void battleSlotFlagSet(int key, int flag)
{
    int handle = battleContextBind(key, 0);

    slotFlagSet(handle, flag);
}

int slotFlagTest(int handle, int value);

/* battleSlotFlagTest: true when flag `flag` of the slot's flag set is set. unitLearnMaskGet calls it. */
int battleSlotFlagTest(int key, int flag)
{
    int handle = battleContextBind(key, 0);

    return slotFlagTest(handle, flag);
}

int slotFlagClear(int handle, int value);

/* battleSlotFlagClear: clears flag `flag` of the slot's flag set. unitLearnMaskSet calls it for each clear bit of a mask. */
void battleSlotFlagClear(int key, int flag)
{
    int handle = battleContextBind(key, 0);

    slotFlagClear(handle, flag);
}

int unitHasEffectCode(int handle, int value);

/* battleUnitHasEffectCode: true when the unit in slot `key` has effect code `code` (unitHasEffectCode). func_00A9B1B8 calls it with code 0x3F. */
int battleUnitHasEffectCode(int key, int code)
{
    int handle = battleContextBind(key, 0);

    return unitHasEffectCode(handle, code);
}

int slotCodeSet(int handle, int value);

/* battleSlotCodeSet: sets effect code `code` (1 to 59) of the slot. configLearnAllApply calls it for keys 1 to 10 and codes 1 to 59. */
void battleSlotCodeSet(int key, int code)
{
    int handle = battleContextBind(key, 0);

    slotCodeSet(handle, code);
}

int slotCodeTest(int handle, int value);

/* battleSlotCodeTest (proposed, medium): calls slotCodeTest on the slot and discards the result. */
void battleSlotCodeTest(int key, int code)
{
    int handle = battleContextBind(key, 0);

    slotCodeTest(handle, code);
}

/*
 * Set, test and clear of the 21-entry mark set at +0x3E of the slot record (func_00A8F4E8 sets,
 * func_00A8F560 tests, func_00A8F5E0 clears; all unnamed). configLearnAllApply tail-calls
 * battleSlotMarkSet with (2, 4). The test wrapper's result is discarded by its C body.
 */
void func_00A8F4E8(BattleContext *context, int value);

/* battleSlotMarkSet (proposed): sets mark `mark` (1 to 21) of the slot's mark set. */
void battleSlotMarkSet(int key, int mark)
{
    func_00A8F4E8(battleContextBind(key, 0), mark);
}

void func_00A8F560(BattleContext *context, int value);

/* battleSlotMarkTest (proposed): reads mark `mark` (1 to 21) of the slot's mark set. */
void battleSlotMarkTest(int key, int mark)
{
    func_00A8F560(battleContextBind(key, 0), mark);
}

void func_00A8F5E0(BattleContext *context, int value);

/* battleSlotMarkClear (proposed): clears mark `mark` (1 to 21) of the slot's mark set. */
void battleSlotMarkClear(int key, int mark)
{
    func_00A8F5E0(battleContextBind(key, 0), mark);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu023", battleSlotsRefresh);
