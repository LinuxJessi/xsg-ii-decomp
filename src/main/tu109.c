#include "common.h"
#include "main/tu109.h"
#include "main/tu121.h"
#include "shared.h"

typedef struct SelectionCounter {
    unsigned char unmodeled_00[8];
    int limit_a;
    int count;
} SelectionCounter;

/* The taken bits of one holder (0x2B bytes): bit s of header[g] marks slot s of group g opened,
   bit b of cells[g][s] its sub b taken; openGroup is the highest open group, takenCount the number
   of taken subs (tu100.c/tu101.c spell these two bytes flags[2]; selectionInit only matches with scalars). */
struct ControlBuffer {
    unsigned char header[4];
    unsigned char cells[4][8];
    unsigned char openGroup;
    unsigned char takenCount;
    /* Values searched by selectionFindIndex. */
    unsigned char entries[4];
    unsigned char entry_count;
};

/* A member record (the type of ptState's slots): it owns a bitmap of taken slot bits and the
   counter their costs are charged to; id is the character looked up with func_A8B490. */
typedef struct SelectionHolder {
    int id;
    struct PartyMemberStatus *status;
    SelectionCounter *counter;
    struct ControlBuffer *bits;
} SelectionHolder;

/* While split_window is nonzero, the second scan skips the values 30..32. */
struct SelectionSettings {
    /* 256-bit eligibility bitmap at block->slots, filled by selectionBlockFill and read by selectionBlockTestBit. */
    struct SelectionBlock *block;
    /* Member slots 0-6 front, 7-9 back. */
    SelectionHolder *slots[10];
    SelectionHolder *roster[10];
    unsigned char split_window;
    unsigned char frontIndex;
    unsigned char backIndex;
    /* Member counts; counts[1] is read outside split mode, counts[2] in it. */
    unsigned char counts[3];
};

/* Bits of selectionFlagTable: when an entry can be chosen (selectionEntryIsEligible)
   and what makes it useful for a member (func_00228F68). */
enum SelectionFlag {
    SELECTION_FLAG_RESTORE_GAUGE = 0x1,   /* useful while HP (value5C) differs from its maximum (gauge) */
    SELECTION_FLAG_RESTORE_ETHER = 0x2,   /* useful while EP (value60) differs from its maximum (value6) */
    SELECTION_FLAG_ALWAYS = 0x4,          /* always useful */
    SELECTION_FLAG_RESTRICTED = 0x8,      /* eligible only while D_003EC664 is 7 or 8 */
    SELECTION_FLAG_DISABLED = 0x10,       /* never eligible */
    SELECTION_FLAG_SPLIT_ONLY = 0x20,     /* eligible only in split mode (without it, only outside) */
    SELECTION_FLAG_ANY_MODE = 0x40,       /* eligible in and outside split mode */
    SELECTION_FLAG_COUNT = 0x80,          /* useful while counter->count is below 99999 */
    SELECTION_FLAG_LIMIT = 0x100          /* useful while counter->limit_a is below 99999 */
};

/* OV01 dataBoxChkCategory1 (config/symbols/ov01.txt). */
int func_A9C7A8(int value);
/* unsigned: selectionWindowListDropAt reads it with lbu and decrements with addiu 0xFF. */
extern unsigned char selectionWindowCount;
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

/* The func_A9C7A8 count of window entry index; an entry without one is dropped from the list (returns 0). */
int selectionWindowListDropAt(int index)
{
    int i;

    if (func_A9C7A8(selectionWindowList[index]) <= 0)
    {
        for (i = index + 1; i < selectionWindowCount; i++)
        {
            selectionWindowList[i - 1] = selectionWindowList[i];
        }
        selectionWindowCount--;
        return 0;
    }
    return func_A9C7A8(selectionWindowList[index]);
}

extern u16 selectionFlagTable[];

void selectionFlagTableInit(void)
{
    int i;
    u16 *flags;

    for (i = 41; i >= 0; i--)
    {
        selectionFlagTable[i] = 0;
    }
    flags = selectionFlagTable;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE;
    *flags++ = SELECTION_FLAG_RESTORE_ETHER;
    *flags++ = SELECTION_FLAG_RESTORE_ETHER;
    *flags++ = SELECTION_FLAG_RESTORE_ETHER;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_ANY_MODE;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER | SELECTION_FLAG_ALWAYS;
    *flags++ = SELECTION_FLAG_LIMIT;
    *flags++ = SELECTION_FLAG_LIMIT;
    *flags++ = SELECTION_FLAG_LIMIT;
    *flags++ = SELECTION_FLAG_LIMIT;
    *flags++ = SELECTION_FLAG_LIMIT;
    *flags++ = SELECTION_FLAG_COUNT;
    *flags++ = SELECTION_FLAG_COUNT;
    *flags++ = SELECTION_FLAG_COUNT;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER | SELECTION_FLAG_SPLIT_ONLY;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER | SELECTION_FLAG_SPLIT_ONLY;
    *flags++ = SELECTION_FLAG_RESTORE_GAUGE | SELECTION_FLAG_RESTORE_ETHER | SELECTION_FLAG_ALWAYS
               | SELECTION_FLAG_SPLIT_ONLY;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_DISABLED;
    *flags++ = SELECTION_FLAG_ALWAYS;
    *flags++ = SELECTION_FLAG_ALWAYS;
    *flags++ = SELECTION_FLAG_ALWAYS;
    *flags = SELECTION_FLAG_ALWAYS;
}

/* The word after ptSplitMode (selectionEntryIsEligible accepts 7..8); an array like
   ptSplitMode, since a scalar extern would be addressed gp-relative under -G8. */
extern int D_003EC664[];

int selectionEntryIsEligible(int index)
{
    int flags;

    flags = selectionFlagTable[selectionWindowList[index]];
    if (flags & SELECTION_FLAG_DISABLED)
    {
        return 0;
    }
    if (flags & SELECTION_FLAG_RESTRICTED)
    {
        if ((unsigned int) (D_003EC664[0] - 7) >= 2)
        {
            return 0;
        }
    }
    if (!(flags & SELECTION_FLAG_ANY_MODE))
    {
        if (ptSplitMode[0] != 0)
        {
            if (!(flags & SELECTION_FLAG_SPLIT_ONLY))
            {
                return 0;
            }
        }
        else if (flags & SELECTION_FLAG_SPLIT_ONLY)
        {
            return 0;
        }
    }
    return 1;
}

int selectionWindowEntryHasFlag4(int index)
{
    if ((selectionFlagTable[selectionWindowList[index]] & 0x4) != 0) {
        return 1;
    }
    return 0;
}

/* Admitted aligned(1) scalar: the gauge word sits at the unaligned offset 2 of the record. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* The character record returned by func_A8B490 (OV01). */
typedef struct MenuPartyRecord {
    unsigned char unmodeled_00[2];
    UnalignedS32 gauge;                     /* 0x02 */
    short value6;                           /* 0x06 */
    unsigned char unmodeled_08[0xB];
    signed char level;                      /* 0x13 */
    unsigned char unmodeled_14[0x48];
    int value5C;                            /* 0x5C */
    short value60;                          /* 0x60 */
} MenuPartyRecord;

/* OV01 battleContextStateGet (config/symbols/ov01.txt). */
MenuPartyRecord *func_A8B490(int id);

/* Whether window entry index is eligible and useful for holder's character (enum SelectionFlag);
   an entry with no bit besides the two split-mode ones is useful. */
int func_00228F68(int index, SelectionHolder *holder)
{
    u16 flags;
    MenuPartyRecord *record;
    int useful;
    /* Holds the 1 every test below stores; with a literal 1 the register allocation differs. */
    int usable;

    flags = selectionFlagTable[selectionWindowList[index]];
    if (selectionEntryIsEligible(index) == 0)
    {
        return 0;
    }
    record = func_A8B490(holder->id);
    flags &= ~(SELECTION_FLAG_SPLIT_ONLY | SELECTION_FLAG_ANY_MODE);
    usable = 1;
    useful = flags == 0;
    if (flags & SELECTION_FLAG_LIMIT)
    {
        if (holder->counter->limit_a < 99999)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_COUNT)
    {
        if (holder->counter->count < 99999)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_RESTORE_GAUGE)
    {
        if (record->gauge != record->value5C)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_RESTORE_ETHER)
    {
        if (record->value6 != record->value60)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_ALWAYS)
    {
        useful = usable;
    }
    return useful;
}

u16 selectionWindowEntryMarkedFlags(int index)
{
    u16 flags;

    flags = selectionFlagTable[selectionWindowList[index]];
    return (flags & 0x180) ? flags : 0;
}

/* Whether a member of the active slots (back 7..9 in split mode, front 0..6 otherwise) is not at full HP. */
int func_00229088(void)
{
    int start;
    int count;
    int i;
    SelectionHolder *member;
    MenuPartyRecord *record;

    if (ptSplitMode[0] != 0)
    {
        start = 7;
        count = 3;
    }
    else
    {
        start = 0;
        count = 7;
    }
    for (i = 0; i < count; i++)
    {
        member = ptState.slots[start + i];
        if (member != 0)
        {
            record = func_A8B490(member->id);
            if (record->value5C != record->gauge)
            {
                return 1;
            }
        }
    }
    return 0;
}

/* Whether one of the counted members (counts[2] back ones while split_window is set, counts[1] front
   ones otherwise) is not at full HP and EP. */
int func_00229130(void)
{
    int start;
    int count;
    int i;
    MenuPartyRecord *record;

    if (ptState.split_window != 0)
    {
        count = ptState.counts[2];
        start = 7;
    }
    else
    {
        count = ptState.counts[1];
        start = 0;
    }
    for (i = 0; i < count; i++)
    {
        record = func_A8B490(ptState.slots[start + i]->id);
        if (record->value5C != record->gauge || record->value60 != record->value6)
        {
            return 1;
        }
    }
    return 0;
}

/* Bit c of slots (bit c & 7 of byte c >> 3) marks candidate c; selectionBlockTestBit tests it. */
typedef struct SelectionBlock {
    unsigned short progressId;              /* 0x00 */
    unsigned char unmodeled_02[2];
    /* xglClockDayTime2UInt reading taken by selectionInit. */
    unsigned int playTime;                  /* 0x04 */
    /* -1 while nothing is attached (tu108.c); selectionInit stores it. */
    int id;                                 /* 0x08 */
    unsigned char unmodeled_0C[0x21 - 0x0C];
    unsigned char mapFlag;                  /* 0x21 */
    unsigned char slots[32];                /* 0x22 */
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

/* OV01 dataBoxChkCategory3 (config/symbols/ov01.txt). */
int func_A9C868(int id);

void selectionBlockFill(void)
{
    int candidate;
    int bit;
    int slot;

    candidate = 1;
    do
    {
        bit = candidate & 7;
        slot = candidate >> 3;
        if (func_A9C868(candidate) != 0)
        {
            ptState.block->slots[slot] |= 1 << bit;
        }
        candidate += 1;
    }
    while (candidate < 255);
}

int selectionBlockTestBit(int slot)
{
    unsigned int value;

    value = ptState.block->slots[slot >> 3];
    return (value >> (slot & 7)) & 1;
}

/* OV01 dataBoxChkCategory2 (config/symbols/ov01.txt). */
int func_A9C808(int value);
/* unsigned: selectionListDropAt reads it with lbu and decrements with addiu 0xFF. */
extern unsigned char selectionPlainCount;
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

/* OV01 dataBoxDecCategory2 (config/symbols/ov01.txt). */
int func_A9C848(int value);

/* Uses up one of plain entry index (func_A9C848) and returns what is left; an entry with none left is
   dropped from the list. */
int selectionListDropAt(int index)
{
    int i;

    func_A9C848(selectionPlainList[index]);
    if (func_A9C808(selectionPlainList[index]) <= 0)
    {
        for (i = index + 1; i < selectionPlainCount; i++)
        {
            selectionPlainList[i - 1] = selectionPlainList[i];
        }
        selectionPlainCount--;
        return 0;
    }
    return func_A9C808(selectionPlainList[index]);
}

struct LookupRecord {
    int value;
    /* The attribute bytes (tu101.c's spelling). In the special-attack records func_00229678
     * and func_00229750 scan, attributes[1] is the copilot member mask (bit n for member id
     * n + 1) and attributes[2] the bit of the caller's mask that enables the record; in the pair
     * records of func_002294B0, attributes[0] and attributes[1] are the two members who perform it. */
    unsigned char attributes[3];
    unsigned char unmodeled_07[9];
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

extern unsigned char D_003EC0D7[];
/* OV01 battleSlotMarkTest (config/symbols/ov01.txt). */
int func_A8B870(int id, int mark);

/* The D_003EC0D7 value of character id; for id 2 it is 3 higher, and 4 higher while its mark 4 is set. */
int func_00229460(int id)
{
    int value;

    value = D_003EC0D7[id];
    if (id == 2)
    {
        value += 3;
        if (func_A8B870(id, 4) != 0)
        {
            value += 1;
        }
    }
    return value;
}

int ptFindSlot(int id);

/* Counts the pair records 79..94 of D_003EBAA8 that are enabled in mask (bit attributes[2]), have member
   as one of their two members and whose other member ptFindSlot finds. */
int func_002294B0(int member, unsigned short mask)
{
    int count;
    int i;

    count = 0;
    for (i = 79; i < 95; i++)
    {
        if ((mask >> D_003EBAA8[i].attributes[2]) & 1)
        {
            if (D_003EBAA8[i].attributes[0] == member)
            {
                if (ptFindSlot(D_003EBAA8[i].attributes[1]) != -1)
                {
                    count++;
                }
            }
            else if (D_003EBAA8[i].attributes[1] == member)
            {
                if (ptFindSlot(D_003EBAA8[i].attributes[0]) != -1)
                {
                    count++;
                }
            }
        }
    }
    return count;
}

/* Index of the nth such pair record, or -1 when there is none. The original masks i with 15,
   although i is below 16 there. */
int func_00229578(int member, int nth, unsigned short mask)
{
    int found;
    int i;

    found = 0;
    for (i = 0; i < 16; i++)
    {
        if ((mask >> D_003EBAA8[i + 79].attributes[2]) & 1)
        {
            if (D_003EBAA8[i + 79].attributes[0] == member)
            {
                if (ptFindSlot(D_003EBAA8[i + 79].attributes[1]) != -1)
                {
                    found++;
                    if (nth + 1 == found)
                    {
                        break;
                    }
                }
            }
            else if (D_003EBAA8[i + 79].attributes[1] == member)
            {
                if (ptFindSlot(D_003EBAA8[i + 79].attributes[0]) != -1)
                {
                    found++;
                    if (nth + 1 == found)
                    {
                        break;
                    }
                }
            }
        }
    }
    if (i == 16)
    {
        return -1;
    }
    return (i & 15) + 79;
}

SelectionHolder *ptFindRecord(int id);

/* Counts the records among member's seven (from D_003EBAA8[member * 10 - 58]) that are enabled in mask
   (bit attributes[2]) and whose copilot member mask includes a member ptFindRecord finds. */
int func_00229678(int member, unsigned short mask)
{
    int present;
    int count;
    int firstRecord;
    int i;

    present = 0;
    for (i = 0; i < 7; i++)
    {
        if (ptFindRecord(i + 1) != 0)
        {
            present |= 1 << i;
        }
    }
    firstRecord = member * 10 - 58;
    count = 0;
    for (i = 0; i < 7; i++)
    {
        if (((mask >> D_003EBAA8[firstRecord + i].attributes[2]) & 1)
            && (D_003EBAA8[firstRecord + i].attributes[1] & present))
        {
            count++;
        }
    }
    return count;
}

/* Index of the nth such record, or -1 when there is none. */
int func_00229750(int member, int nth, unsigned short mask)
{
    int present;
    int found;
    int firstRecord;
    int i;

    present = 0;
    for (i = 0; i < 7; i++)
    {
        if (ptFindRecord(i + 1) != 0)
        {
            present |= 1 << i;
        }
    }
    firstRecord = member * 10 - 58;
    found = 0;
    for (i = 0; i < 7; i++)
    {
        if (((mask >> D_003EBAA8[firstRecord + i].attributes[2]) & 1)
            && (D_003EBAA8[firstRecord + i].attributes[1] & present))
        {
            found++;
            if (nth + 1 == found)
            {
                break;
            }
        }
    }
    if (i == 7)
    {
        return -1;
    }
    return firstRecord + i;
}

void func_00229898(void)
{
}

int selectionRecordIndex(int group, int slot, int sub)
{
    return (group << 5) + (slot * 4) + sub;
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
char *func_00A9B850(int dispatch_value, int value);
char *func_00A9B750(int dispatch_value, int value);

/* Returns the text pointer of the OV01 call (tu101 draws it with textDrawSegment). */
char *selectionRecordSetValue(int index, int value)
{
    SelectionRecord *record;

    record = &selectionRecordTable[index];
    if (record->kind < 2)
    {
        return func_00A9B850(record->dispatch_value, value);
    }
    else
    {
        return func_00A9B750(record->dispatch_value, value);
    }
}

char *selectionSetValue(int group, int slot, int sub, int value)
{
    return selectionRecordSetValue(selectionRecordIndex(group, slot, sub), value);
}

u16 selectionGetValue(int group, int slot, int sub)
{
    return selectionRecordTable[selectionRecordIndex(group, slot, sub)].value;
}

/* Group 3 has only four slots: its slots 4..7 count as taken. */
int func_00229978(SelectionHolder *holder, int group, int slot, int sub)
{
    if (group == 3 && slot >= 4)
    {
        return 1;
    }
    return ((unsigned int) holder->bits->cells[group][slot] >> sub) & 1;
}

/* OV01 battleSlotFlagTest and battleSlotCodeTest (config/symbols/ov01.txt). */
int func_A8B750(int id, int flag);
int func_A8B810(int id, int code);

/* Whether holder has entry index: the taken bit of a selectionRecordIndex value, or for the extra entries
   func_00229C60 appends, flag 0x3F (0x70) or code index - 0x3D (0x71 and up); 0x7F is never held. */
int func_002299B0(SelectionHolder *holder, int index)
{
    if (index == 0x7F)
    {
        return 0;
    }
    if (index == 0x70)
    {
        return func_A8B750(holder->id, 0x3F) != 0;
    }
    if (index > 0x70)
    {
        return func_A8B810(holder->id, index - 0x3D) != 0;
    }
    return ((unsigned int) holder->bits->cells[index >> 5][(index >> 2) & 7] >> (index & 3)) & 1;
}

int selectionEntryBitCount(SelectionHolder *holder, int group, int slot)
{
    int sum_zero;
    int sum_three;
    int sum_two;

    sum_zero = func_00229978(holder, group, slot, 0);
    sum_three = func_00229978(holder, group, slot, 3);
    sum_two = func_00229978(holder, group, slot, 2);
    sum_zero += sum_three;
    sum_two += func_00229978(holder, group, slot, 1);
    return sum_zero + sum_two;
}

int selectionRowBitCount(SelectionHolder *holder, int group)
{
    int value_0;
    int value_1;
    int value_2;
    int value_3;
    int value_4;
    int value_5;
    int value_6;
    int value_7;

    value_0 = selectionEntryBitCount(holder, group, 0);
    value_7 = selectionEntryBitCount(holder, group, 7);
    value_3 = selectionEntryBitCount(holder, group, 3);
    value_5 = selectionEntryBitCount(holder, group, 5);
    value_1 = selectionEntryBitCount(holder, group, 1);
    value_2 = selectionEntryBitCount(holder, group, 2);
    value_4 = selectionEntryBitCount(holder, group, 4);
    value_6 = selectionEntryBitCount(holder, group, 6);
    value_3 += value_5;
    value_4 += value_6;
    value_0 += value_7;
    value_1 += value_2;
    value_0 += value_3;
    value_1 += value_4;
    return value_0 + value_1;
}

int selectionFindIndex(SelectionHolder *holder, int value)
{
    int found;
    int i;

    found = -1;
    for (i = 0; i < holder->bits->entry_count; i++)
    {
        if (holder->bits->entries[i] == value)
        {
            found = i;
            break;
        }
    }
    return found;
}

/* Fills list with the record indices the holder has taken whose record kind is kind (kind 3: every index,
   taken or not), then the extra entries it holds (0x70 for kind 1; 0x71..0x76 for kind 2 of the
   character with id 3), and returns the count. */
int func_00229C60(unsigned char *list, SelectionHolder *holder, int kind)
{
    int group;
    int slot;
    int sub;
    int count;

    count = 0;
    for (group = 0; group < 4; group++)
    {
        for (slot = 0; (group < 3) ? (slot < 8) : (slot < 4); slot++)
        {
            for (sub = 0; sub < 4; sub++)
            {
                if (kind == 3 || func_00229978(holder, group, slot, sub) != 0)
                {
                    list[count] = selectionRecordIndex(group, slot, sub);
                    if (kind == 3 || selectionRecordTable[list[count]].kind == kind)
                    {
                        count++;
                    }
                }
            }
        }
    }
    if (func_A8B750(holder->id, 0x3F) != 0 && kind == 1)
    {
        list[count++] = 0x70;
    }
    if (kind == 2 && holder->id == 3)
    {
        /* The group counter is reused for the extra values 0x34..0x39: the original keeps them in
           group's register, and a separate variable is allocated another one. */
        for (group = 0x34; group < 0x3A; group++)
        {
            if (func_A8B810(holder->id, group) != 0)
            {
                list[count++] = group + 0x3D;
            }
        }
    }
    return count;
}

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

/* Whether every sub of group counts as taken: 32 of them, or 16 for group 3, which the 16 that
   func_00229978 reports for its missing slots 4..7 already reach. */
int func_00229E68(SelectionHolder *holder, int group)
{
    int count;

    count = selectionRowBitCount(holder, group);
    if (group == 3)
    {
        if (count >= 16)
        {
            return 1;
        }
    }
    else if (count >= 32)
    {
        return 1;
    }
    return 0;
}

extern u16 D_003EC0E8[];

/* Whether group can open slot: not taken yet, group open, and its D_003EC0E8 cost within counter->count. */
int func_00229EC0(SelectionHolder *holder, int group, int slot)
{
    if (((holder->bits->header[group] >> slot) & 1) || holder->bits->openGroup < group
        || holder->counter->count < D_003EC0E8[group * 8 + slot])
    {
        return 0;
    }
    return 1;
}

/* Whether sub of (group, slot) can be taken: not taken yet, slot open, and its record value within counter->limit_a. */
int func_00229F30(SelectionHolder *holder, int group, int slot, int sub)
{
    if (((holder->bits->cells[group][slot] >> sub) & 1) || !((holder->bits->header[group] >> slot) & 1)
        || holder->counter->limit_a < selectionRecordTable[selectionRecordIndex(group, slot, sub)].value)
    {
        return 0;
    }
    return 1;
}

/* Takes sub of (group, slot) and pays its record value; returns 1 once all four subs of the slot are taken,
   opening the next group when the slot belongs to the highest open one. */
int func_00229FD8(SelectionHolder *holder, int group, int slot, int sub)
{
    int index;
    SelectionCounter *counter;
    int open;

    holder->bits->cells[group][slot] |= 1 << sub;
    index = selectionRecordIndex(group, slot, sub);
    counter = holder->counter;
    counter->limit_a -= selectionRecordTable[index].value;
    holder->bits->takenCount++;
    if (holder->bits->cells[group][slot] != 0xF)
    {
        return 0;
    }
    open = holder->bits->openGroup;
    if (group >= open && group < 3)
    {
        holder->bits->openGroup = open + 1;
    }
    return 1;
}

int selectionRecordIndex(int group, int slot, int sub);
/* OV01 battleSlotFlagSet and battleSlotCodeSet (config/symbols/ov01.txt). */
void func_A8B720(int handle, unsigned char value);
void func_A8B7E0(int handle, unsigned char value);

int selectionApplyEntry(SelectionHolder *holder, int first, int second, int third)
{
    int index;
    SelectionRecord *entry;

    index = selectionRecordIndex(first, second, third);
    entry = &selectionRecordTable[index];
    if (entry->kind == 2)
    {
        func_A8B7E0(holder->id, entry->dispatch_value);
    }
    else
    {
        func_A8B720(holder->id, entry->dispatch_value);
    }
    return func_00229FD8(holder, first, second, third);
}

void selectionHolderCharge(SelectionHolder *holder, int slot, int bit)
{
    SelectionCounter *counter;

    holder->bits->header[slot] |= 1 << bit;
    counter = holder->counter;
    counter->count -= D_003EC0E8[slot * 8 + bit];
}

/* The clock reading xglClockRead fills. */
typedef struct DayTime {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} DayTime;

/* The ten holders' buffers (ptMemberRecords[i].bits); tu108.c and tu110.c view them as SaveMemberName. */
extern struct ControlBuffer D_006A5880[];
extern SelectionHolder ptMemberRecords[];
/* The block ptState.block points at. */
extern SelectionBlock D_006A5A30;
/* OV01 battleSlotsInit, dataBoxClearAll, sdvInitSpecialWork and dataBoxIncCategory3 (config/symbols/ov01.txt). */
void func_A8AFC8(void);
void func_A9C610(void);
void func_A91A78(void);
void func_A9C888(int id);
void cursorBufferTablesInit(void);
signed char saveTextMaximaCopy(signed char *maxima);
void saveTextStateReset(void);
void xglClockRead(DayTime *time);
unsigned int xglClockDayTime2UInt(DayTime *time);

/* Clears the ten holders' taken bits and gives their counters 600 points, takes the first two subs of
   (0, 0) for the member with id 4, resets the selection state, adds 30 points per saved maximum when
   saveTextMaximaCopy reports them, fills the eligibility bitmap and stamps the play time. */
void selectionInit(void)
{
    signed char maxima[6];
    DayTime now;
    int member;
    int j;
    int k;
    SelectionRecord *entry;

    func_A8AFC8();
    func_A9C610();
    func_A91A78();
    member = 0;
    do
    {
        for (j = 0; j < 4; j++)
        {
            D_006A5880[member].header[j] = 0;
            for (k = 0; k < 8; k++)
            {
                D_006A5880[member].cells[j][k] = 0;
            }
            D_006A5880[member].entries[j] = 0;
        }
        D_006A5880[member].openGroup = 0;
        D_006A5880[member].takenCount = 0;
        D_006A5880[member].entry_count = 0;
        ptMemberRecords[member].counter->limit_a = 600;
        ptMemberRecords[member].counter->count = 600;
        ptState.slots[member] = 0;
        ptState.roster[member] = 0;
        if (ptMemberRecords[member].id == 4)
        {
            ptMemberRecords[member].bits->header[0] |= 1;
            ptMemberRecords[member].bits->cells[0][0] |= 3;
            ptMemberRecords[member].bits->takenCount += 2;
            entry = &selectionRecordTable[selectionRecordIndex(0, 0, 0)];
            if (entry->kind == 2)
            {
                func_A8B7E0(4, entry->dispatch_value);
            }
            else
            {
                func_A8B720(4, entry->dispatch_value);
            }
            entry = &selectionRecordTable[selectionRecordIndex(0, 0, 1)];
            if (entry->kind == 2)
            {
                func_A8B7E0(4, entry->dispatch_value);
            }
            else
            {
                func_A8B720(4, entry->dispatch_value);
            }
        }
        member++;
    }
    while (member < 10);
    D_006A5A30.mapFlag = 0;
    ptState.split_window = 0;
    ptState.frontIndex = 0;
    ptState.backIndex = 0;
    ptState.counts[0] = 0;
    ptState.counts[1] = 0;
    ptState.counts[2] = 0;
    ptState.block->id = -1;
    D_006A5A30.progressId = 0;
    cursorBufferTablesInit();
    selectionBlockClear();
    if (saveTextMaximaCopy(maxima) & 3)
    {
        ptMemberRecords[0].counter->limit_a += maxima[0] * 30;
        ptMemberRecords[1].counter->limit_a += maxima[1] * 30;
        ptMemberRecords[2].counter->limit_a += maxima[2] * 30;
        ptMemberRecords[4].counter->limit_a += maxima[3] * 30;
        ptMemberRecords[5].counter->limit_a += maxima[4] * 30;
        ptMemberRecords[6].counter->limit_a += maxima[5] * 30;
        func_A8B720(5, 0x3F);
        func_A8B720(2, 0x3F);
        func_A9C888(0x45);
    }
    saveTextStateReset();
    selectionBlockFill();
    xglClockRead(&now);
    ptState.block->playTime = xglClockDayTime2UInt(&now);
}
