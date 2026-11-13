#include "common.h"
#include "main/tu109.h"
#include "main/tu121.h"

/* While split_window is nonzero, the second scan skips the values 30..32. */
struct SelectionSettings {
    /* 256-bit eligibility bitmap at block->slots, filled by selectionBlockFill and read by selectionBlockTestBit. */
    struct SelectionBlock *block;
    unsigned char unmodeled_04[80];
    unsigned char split_window;
};

int func_A9C7A8(int value);
extern signed char selectionWindowCount;
extern struct SelectionSettings ptState;
extern unsigned char ptSplitMode[];
extern unsigned char selectionWindowList[];

void selectionRebuildWindowList(void)
{
    int candidate;
    int count;

    count = 0;
    if (ptSplitMode[0] != 0)
    {
        candidate = 30;
        do
        {
            if (func_A9C7A8(candidate) > 0)
            {
                selectionWindowList[count] = candidate;
                count += 1;
            }
            candidate += 1;
        }
        while (candidate < 33);
    }
    candidate = 0;
    do
    {
        if ((ptState.split_window == 0 || (unsigned int) (candidate - 30) >= 3) && func_A9C7A8(candidate) > 0)
        {
            selectionWindowList[count] = candidate;
            count += 1;
        }
        candidate += 1;
    }
    while (candidate < 255);
    selectionWindowCount = count;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionWindowListDropAt);

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionFlagTableInit);

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionEntryIsEligible);

#include "shared.h"

extern u16 selectionFlagTable[];

int selectionWindowEntryHasFlag4(int index)
{
    if ((selectionFlagTable[selectionWindowList[index]] & 0x4) != 0) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00228F68);

u16 selectionWindowEntryMarkedFlags(int index)
{
    u16 flags;

    flags = selectionFlagTable[selectionWindowList[index]];
    return (flags & 0x180) ? flags : 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229088);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229130);

/* Bit c of slots (bit c & 7 of byte c >> 3) marks candidate c; selectionBlockTestBit tests it. */
typedef struct SelectionBlock {
    unsigned char unmodeled_00[34];
    unsigned char slots[32];
} SelectionBlock;

void selectionBlockClear(void)
{
    int slot;

    slot = 0;
    do {
        ptState.block->slots[slot] = 0;
        slot++;
    } while (slot < 32);
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionBlockFill);

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionBlockTestBit);

int func_A9C808(int value);
extern signed char selectionPlainCount;
extern unsigned char selectionPlainList[];

void selectionRebuildList(void)
{
    int candidate;
    int count;

    candidate = 0;
    count = 0;
    do
    {
        if (func_A9C808(candidate) > 0)
        {
            selectionPlainList[count] = candidate;
            count += 1;
        }
        candidate += 1;
    }
    while (candidate < 255);
    selectionPlainCount = count;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionListDropAt);

struct LookupRecord {
    int value;
    unsigned char unmodeled_04[12];
};

extern struct LookupRecord D_003EBAA8[];
extern unsigned char menuMainTxdBuffer[];

void selectionBuildKeyTable(void)
{
    int index;
    int key;
    struct LookupRecord *record;

    index = 0;
    record = D_003EBAA8;
    do
    {
        key = index + 0x50;
        index += 1;
        record->value = txdTableEntry(menuMainTxdBuffer, key);
        record += 1;
    }
    while (index < 99);
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229460);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_002294B0);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229578);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229678);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229750);

void func_00229898(void)
{
}

int selectionRecordIndex(int group, int slot, int sub)
{
    return (group << 5) + (slot * 4) + sub;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionRecordSetValue);


char *selectionSetValue(int group, int slot, int sub, int value)
{
    return selectionRecordSetValue(selectionRecordIndex(group, slot, sub), value);
}

/* One entry per (group, slot, sub), indexed by selectionRecordIndex: kind selects the device call (2 or not),
   dispatch_value is its argument, value is the stored result and bit is the flag tested by selectionEntryBitOutsideMask. */
typedef struct SelectionRecord {
    unsigned char kind;
    unsigned char dispatch_value;
    unsigned short value;
    unsigned char bit;
    unsigned char unmodeled_05;
} SelectionRecord;

extern SelectionRecord selectionRecordTable[];

u16 selectionGetValue(int group, int slot, int sub)
{
    return selectionRecordTable[selectionRecordIndex(group, slot, sub)].value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229978);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_002299B0);

int func_00229978(int first, int second, int third, int selector);

int selectionEntryBitCount(int first, int second, int third)
{
    int sum_zero;
    int sum_three;
    int sum_two;

    sum_zero = func_00229978(first, second, third, 0);
    sum_three = func_00229978(first, second, third, 3);
    sum_two = func_00229978(first, second, third, 2);
    sum_zero += sum_three;
    sum_two += func_00229978(first, second, third, 1);
    return sum_zero + sum_two;
}

int selectionRowBitCount(int first, int second)
{
    int value_0;
    int value_1;
    int value_2;
    int value_3;
    int value_4;
    int value_5;
    int value_6;
    int value_7;

    value_0 = selectionEntryBitCount(first, second, 0);
    value_7 = selectionEntryBitCount(first, second, 7);
    value_3 = selectionEntryBitCount(first, second, 3);
    value_5 = selectionEntryBitCount(first, second, 5);
    value_1 = selectionEntryBitCount(first, second, 1);
    value_2 = selectionEntryBitCount(first, second, 2);
    value_4 = selectionEntryBitCount(first, second, 4);
    value_6 = selectionEntryBitCount(first, second, 6);
    value_3 += value_5;
    value_4 += value_6;
    value_0 += value_7;
    value_1 += value_2;
    value_0 += value_3;
    value_1 += value_4;
    return value_0 + value_1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionFindIndex);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229C60);

int selectionEntryBitOutsideMask(int group, int slot, int sub, int mask)
{
    SelectionRecord *table;
    SelectionRecord *record;
    int clear_bits;

    table = selectionRecordTable;
    record = &table[selectionRecordIndex(group, slot, sub)];
    clear_bits = ~mask;
    clear_bits &= 1 << record->bit;
    return clear_bits != 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229E68);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229EC0);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229F30);

INCLUDE_ASM("asm/main/nonmatchings/tu109", func_00229FD8);

int selectionRecordIndex(int group, int slot, int sub);
int func_00229FD8(int *target, int first, int second, int third);
void func_A8B720(int handle, unsigned char value);
void func_A8B7E0(int handle, unsigned char value);

int selectionApplyEntry(int *target, int first, int second, int third)
{
    int index;
    SelectionRecord *entry;

    index = selectionRecordIndex(first, second, third);
    entry = &selectionRecordTable[index];
    if (entry->kind == 2)
    {
        func_A8B7E0(*target, entry->dispatch_value);
    }
    else
    {
        func_A8B720(*target, entry->dispatch_value);
    }
    return func_00229FD8(target, first, second, third);
}

extern u16 D_003EC0E8[];

typedef struct SelectionCounter {
    unsigned char unmodeled_00[12];
    int count;
} SelectionCounter;

/* A holder owns a bitmap of taken slot bits and the counter their costs are charged to. */
typedef struct SelectionHolder {
    unsigned char unmodeled_00[8];
    SelectionCounter *counter;
    unsigned char *bits;
} SelectionHolder;

void selectionHolderCharge(SelectionHolder *holder, int slot, int bit)
{
    SelectionCounter *counter;

    holder->bits[slot] |= 1 << bit;
    counter = holder->counter;
    counter->count -= D_003EC0E8[slot * 8 + bit];
}

INCLUDE_ASM("asm/main/nonmatchings/tu109", selectionInit);
