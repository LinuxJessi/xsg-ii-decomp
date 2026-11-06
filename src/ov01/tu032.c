/*
 * OV01 original TU 32: 0x00a91910..0x00a92150 (21 functions)
 */
#include "common.h"
#include "ov01/tu005.h"
#include "ov01/tu032.h"

struct Ov01Entry {
    unsigned char unmodeled_00[8];
    unsigned short flags;
};

struct Ov01Link {
    unsigned char unmodeled_00[0x16];
    signed char level;
};

struct Ov01Owner {
    unsigned char unmodeled_00[0x144];
    struct Ov01Link *link;
};

struct Ov01Entry *func_00A9B3B8(int);

/* Level-scaled chance roll for entry `index`: 0 when the entry has flag 4, else the roll of ov01RollPercent with the owner's level, capped at 100. */
int actLevelChanceRoll(struct Ov01Owner *owner, int index)
{
    struct Ov01Entry *entry;
    signed char level;
    int percent;

    entry = func_00A9B3B8(index);
    if (entry->flags & 4) {
        return 0;
    }
    percent = 0x64;
    level = owner->link->level;
    if (level < 0x65) {
        percent = level;
    }
    return ov01RollPercent(percent) != 0;
}

/* The step counter actStepCounter: step runs 0..3; at step 3 `hold` is a random value of 4 or more. func_00A91978 writes further words past +0x4. */
struct ActStepCounter {
    int step;
    int hold;
};

extern struct ActStepCounter actStepCounter;

struct ActStepCounter *actStepCounterGet(void)
{
    return &actStepCounter;
}

/* Sets the step counter to `step`; at step 3 it draws a hold of rnd() + 4 into +0x4, then draws a class 2, 4 or 10 by rnd(100) into +0x8 and clears +0xC..+0x14. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", func_00A91978);

/* The current phase: the step, or the hold when the step is 3. */
int actStepCounterValue(void)
{
    struct ActStepCounter *counter;
    int step;

    counter = actStepCounterGet();
    step = counter->step;
    if (step != 3) {
        return step;
    }
    return counter->hold;
}

int func_00A91978(int step);

/* Advances the step counter modulo 4 and sets it through func_00A91978. */
void actStepCounterAdvance(void)
{
    struct ActStepCounter *counter;
    int next_step;

    counter = actStepCounterGet();
    next_step = counter->step + 1;
    counter->step = next_step;
    if (next_step >= 4) {
        counter->step = 0;
    }
    func_00A91978(counter->step);
}

void *memset(void *, int, unsigned int);
extern unsigned char sdvSpecialBuf[];

void sdvInitSpecialWork(void)
{
    memset(&sdvSpecialBuf, 0, 0x10);
}

/* Sets bit (index - 16) of the 16-byte table sdvSpecialBuf; indexes outside 16..143 are ignored. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", sdvSpecialBitSet);

/* Returns bit (index - 16) of sdvSpecialBuf, or 0 outside 16..143. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", sdvSpecialBitGet);

extern unsigned char actEntryFlags[];

/* Marks entry `index` in the per-entry byte table actEntryFlags; the index is the key of func_00A9B3B8. */
void actEntryFlagSet(int index)
{
    actEntryFlags[index] = 1;
}

/* Reads the per-entry byte table actEntryFlags at `index`. */
signed char actEntryFlagGet(int index)
{
    return (signed char) actEntryFlags[index];
}

extern unsigned char D_00AC2AC8[];

/* Marks slot index - 0x37 (slots 0..2) of the table D_00AC2AC8. */
void func_00A91B68(int index)
{
    unsigned int slot;

    slot = index - 0x37;
    if (slot < 3U) {
        D_00AC2AC8[slot] = 1;
    }
}

/* Reads slot index - 0x37 of D_00AC2AC8; 0 outside slots 0..2. */
signed char func_00A91B90(int index)
{
    unsigned int slot;

    slot = index - 0x37;
    if (!(slot < 3U)) {
        return 0;
    }
    return (signed char) D_00AC2AC8[slot];
}

/* Resets the action-stock record: stock[4] and boost[2] cleared, max, pickCursor and lastPicked 0, pickSource 1. Unlike the X1 actStockInit it does not recompute the limit. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockReset);

/*
 * The action-stock record (the global actStock, X1 ActStock layout): stock[4] at +0x00,
 * boost[2] at +0x10, max at +0x18, pickCursor at +0x1C, lastPicked at +0x20, pickSource at +0x24.
 * Only boost1 (+0x14, the second boost slot) is declared here.
 */
struct ActStock {
    unsigned char unmodeled_00[0x14];
    int boost1;
};

extern struct ActStock actStock;

/* Returns the action-stock record. */
struct ActStock *actStockGet(void)
{
    return &actStock;
}

/* Half the side-2 units that pass the battle check (flag 4 skipped), rounded up, capped at 4: the action-stock limit. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockMaxGet);

/* Nonzero when the stock is used up: pickCursor at or past max and both boost slots empty. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockNeedsRefill);

/* Refills the action stock: limit from actStockMaxGet, cursor and stock cleared, then the side-2 units that pass the checks and lack flag 8 are stocked and flagged 8. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockMake);

/* Hands out the next acting unit: refills when actStockNeedsRefill says so, takes boost1 and boost2 first, then the stock in order, skips units with flag 4, keeps the pick in lastPicked. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockPick);

/* Drops `unit` from the two boost slots or from the stock; *index receives its stock position, or -1. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockRemove);

/* Drops `unit` from the stock only; *index receives its stock position, or -1. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu032", actStockRemoveStock);

/* Clears the second boost slot (boost1); eventActStockBoostClear calls it. */
void actStockClearBoost1(void)
{
    actStockGet()->boost1 = 0;
}
