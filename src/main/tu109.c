#include "common.h"
#include "main/tu109.h"
#include "main/tu121.h"
#include "ov01/tu023.h"
#include "ov01/tu032.h"
#include "ov01/tu043.h"
#include "shared.h"
#include "main/tu100.h"

/* The points block of a member (tu100.c/tu101.c's ControlLimits): menuDrawClassInfo (tu101.c)
   draws skillPoints beside "S.Pt" and classPoints beside "C.Pt". Taking a sub costs skill points
   (selectionTakeSub), opening a slot class points (selectionHolderCharge). */
typedef struct SelectionCounter {
    unsigned char unmodeled_00[8];
    int skillPoints;                        /* 0x08 */
    int classPoints;                        /* 0x0C */
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
   counter their costs are charged to; id is the character looked up with battleContextStateGet. */
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
   and what makes it useful for a member (selectionEntryIsUseful). */
enum SelectionFlag {
    SELECTION_FLAG_RESTORE_HP = 0x1,      /* useful while hp differs from maxHp */
    SELECTION_FLAG_RESTORE_EP = 0x2,      /* useful while ep differs from maxEp */
    SELECTION_FLAG_ALWAYS = 0x4,          /* always useful */
    SELECTION_FLAG_RESTRICTED = 0x8,      /* eligible only while D_003EC664 is 7 or 8 */
    SELECTION_FLAG_DISABLED = 0x10,       /* never eligible */
    SELECTION_FLAG_SPLIT_ONLY = 0x20,     /* eligible only in split mode (without it, only outside) */
    SELECTION_FLAG_ANY_MODE = 0x40,       /* eligible in and outside split mode */
    SELECTION_FLAG_CLASS_POINTS = 0x80,   /* useful while classPoints is below SELECTION_POINTS_MAX */
    SELECTION_FLAG_SKILL_POINTS = 0x100   /* useful while skillPoints is below SELECTION_POINTS_MAX */
};

/* The window entries 30..32, listed first in split mode and skipped by the second scan while
   split_window is set. */
#define SELECTION_SPLIT_ENTRY_FIRST 30
#define SELECTION_SPLIT_ENTRY_END 33
/* The D_003EC664 values 7 and 8 under which SELECTION_FLAG_RESTRICTED entries are eligible. */
#define SELECTION_RESTRICTED_STATE_FIRST 7
#define SELECTION_RESTRICTED_STATE_COUNT 2
/* The point ceiling (tu100.c clamps classPoints to it). */
#define SELECTION_POINTS_MAX 99999
/* Skill and class points every member starts with, and the skill points added per saved maximum. */
#define SELECTION_START_POINTS 600
#define SELECTION_POINTS_PER_MAXIMUM 30
/* cells[group][slot] once its four subs are taken. */
#define SELECTION_SLOT_ALL_SUBS 0xF
/* The pair records of D_003EBAA8 (two members each) that selectionPairRecordCount scans. */
#define SELECTION_PAIR_RECORD_FIRST 79
#define SELECTION_PAIR_RECORD_COUNT 16

/* Entry values past the selectionRecordIndex range (0x00..0x6F) that selectionCollectEntries
   appends: SELECTION_ENTRY_EXTRA_FLAG stands for battle-slot flag SELECTION_EXTRA_FLAG, the entries
   from SELECTION_ENTRY_EXTRA_CODES for the battle-slot codes SELECTION_EXTRA_CODE_FIRST..END - 1,
   and SELECTION_ENTRY_NONE is held by nobody. */
#define SELECTION_ENTRY_EXTRA_FLAG 0x70
#define SELECTION_ENTRY_EXTRA_CODES 0x71
#define SELECTION_ENTRY_NONE 0x7F
/* The flag selectionInit sets for members 2 and 5 when saveTextMaximaCopy reports saved maxima. */
#define SELECTION_EXTRA_FLAG 0x3F
#define SELECTION_EXTRA_CODE_FIRST 0x34
#define SELECTION_EXTRA_CODE_END 0x3A
/* Entry value minus battle-slot code (0x3D). */
#define SELECTION_EXTRA_CODE_ENTRY_OFFSET (SELECTION_ENTRY_EXTRA_CODES - SELECTION_EXTRA_CODE_FIRST)

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
        candidate = SELECTION_SPLIT_ENTRY_FIRST;
        do
        {
            if (dataBoxChkCategory1(candidate) > 0)
            {
                selectionWindowList[count] = candidate;
                count += 1;
            }
            candidate += 1;
        }
        while (candidate < SELECTION_SPLIT_ENTRY_END);
    }
    candidate = 0;
    do
    {
        if ((ptState.split_window == 0 || (unsigned int) (candidate - SELECTION_SPLIT_ENTRY_FIRST)
             >= SELECTION_SPLIT_ENTRY_END - SELECTION_SPLIT_ENTRY_FIRST) && dataBoxChkCategory1(candidate) > 0)
        {
            selectionWindowList[count] = candidate;
            count += 1;
        }
        candidate += 1;
    }
    while (candidate < 255);
    selectionWindowCount = count;
}

/* The dataBoxChkCategory1 count of window entry index; an entry without one is dropped from the list (returns 0). */
int selectionWindowListDropAt(int index)
{
    int i;

    if (dataBoxChkCategory1(selectionWindowList[index]) <= 0)
    {
        for (i = index + 1; i < selectionWindowCount; i++)
        {
            selectionWindowList[i - 1] = selectionWindowList[i];
        }
        selectionWindowCount--;
        return 0;
    }
    return dataBoxChkCategory1(selectionWindowList[index]);
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
    *flags++ = SELECTION_FLAG_RESTORE_HP;
    *flags++ = SELECTION_FLAG_RESTORE_HP;
    *flags++ = SELECTION_FLAG_RESTORE_HP;
    *flags++ = SELECTION_FLAG_RESTORE_HP;
    *flags++ = SELECTION_FLAG_RESTORE_EP;
    *flags++ = SELECTION_FLAG_RESTORE_EP;
    *flags++ = SELECTION_FLAG_RESTORE_EP;
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP;
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP;
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
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP | SELECTION_FLAG_ALWAYS;
    *flags++ = SELECTION_FLAG_SKILL_POINTS;
    *flags++ = SELECTION_FLAG_SKILL_POINTS;
    *flags++ = SELECTION_FLAG_SKILL_POINTS;
    *flags++ = SELECTION_FLAG_SKILL_POINTS;
    *flags++ = SELECTION_FLAG_SKILL_POINTS;
    *flags++ = SELECTION_FLAG_CLASS_POINTS;
    *flags++ = SELECTION_FLAG_CLASS_POINTS;
    *flags++ = SELECTION_FLAG_CLASS_POINTS;
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP | SELECTION_FLAG_SPLIT_ONLY;
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP | SELECTION_FLAG_SPLIT_ONLY;
    *flags++ = SELECTION_FLAG_RESTORE_HP | SELECTION_FLAG_RESTORE_EP | SELECTION_FLAG_ALWAYS
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
        if ((unsigned int) (D_003EC664[0] - SELECTION_RESTRICTED_STATE_FIRST) >= SELECTION_RESTRICTED_STATE_COUNT)
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
    if ((selectionFlagTable[selectionWindowList[index]] & SELECTION_FLAG_ALWAYS) != 0) {
        return 1;
    }
    return 0;
}

/* Admitted aligned(1) scalar: maxHp sits at the unaligned offset 2 of the record. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* The character record returned by battleContextStateGet (tu101.c's typedef). HP, EP and their
   maxima as src/ov01/tu026.c reads them: HP at +0x5C with its maximum the unaligned int at +0x2,
   EP at +0x60 with its maximum the halfword at +0x6. */
typedef struct MenuPartyRecord {
    unsigned char unmodeled_00[2];
    UnalignedS32 maxHp;                     /* 0x02 */
    short maxEp;                            /* 0x06 */
    short strength;                         /* 0x08: "Str" ("Pow" in split mode) */
    short vitality;                         /* 0x0A: "Vit" ("Arm" in split mode) */
    short etherAttack;                      /* 0x0C: "Eatk" */
    short etherDefense;                     /* 0x0E: "Edef" */
    signed char dexterity;                  /* 0x10: "Dex" */
    signed char evasion;                    /* 0x11: "Eva" */
    signed char agility;                    /* 0x12: "Agl" */
    signed char level;                      /* 0x13 */
    unsigned char unmodeled_14[0x48];
    int hp;                                 /* 0x5C */
    short ep;                               /* 0x60 */
    unsigned char unmodeled_62[0x108 - 0x62]; /* menuDrawCharStats copies 0x108 bytes */
} MenuPartyRecord;

/* OV01 battleContextStateGet (config/symbols/ov01.txt): the state record (BattleState in
   ov01/tu023.c, 0x108 bytes) of the member bound to slot key. */
MenuPartyRecord *battleContextStateGet(int key);

/* Whether window entry index is eligible and useful for holder's character (enum SelectionFlag);
   an entry with no bit besides the two split-mode ones is useful. */
int selectionEntryIsUseful(int index, SelectionHolder *holder)
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
    record = battleContextStateGet(holder->id);
    flags &= ~(SELECTION_FLAG_SPLIT_ONLY | SELECTION_FLAG_ANY_MODE);
    usable = 1;
    useful = flags == 0;
    if (flags & SELECTION_FLAG_SKILL_POINTS)
    {
        if (holder->counter->skillPoints < SELECTION_POINTS_MAX)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_CLASS_POINTS)
    {
        if (holder->counter->classPoints < SELECTION_POINTS_MAX)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_RESTORE_HP)
    {
        if (record->maxHp != record->hp)
        {
            useful = usable;
        }
    }
    if (flags & SELECTION_FLAG_RESTORE_EP)
    {
        if (record->maxEp != record->ep)
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
    return (flags & (SELECTION_FLAG_CLASS_POINTS | SELECTION_FLAG_SKILL_POINTS)) ? flags : 0;
}

/* Whether a member of the active slots (back 7..9 in split mode, front 0..6 otherwise) is not at full HP. */
int selectionActiveMemberNeedsHp(void)
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
            record = battleContextStateGet(member->id);
            if (record->hp != record->maxHp)
            {
                return 1;
            }
        }
    }
    return 0;
}

/* Whether one of the counted members (counts[2] back ones while split_window is set, counts[1] front
   ones otherwise) has HP or EP below its maximum. */
int selectionCountedMemberNeedsHpEp(void)
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
        record = battleContextStateGet(ptState.slots[start + i]->id);
        if (record->hp != record->maxHp || record->ep != record->maxEp)
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
int dataBoxChkCategory3(int id);

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
        if (dataBoxChkCategory3(candidate) != 0)
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
int dataBoxChkCategory2(int value);
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
        if (dataBoxChkCategory2(candidate) > 0)
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
int dataBoxDecCategory2(int value);

/* Uses up one of plain entry index (dataBoxDecCategory2) and returns what is left; an entry with none left is
   dropped from the list. */
int selectionListDropAt(int index)
{
    int i;

    dataBoxDecCategory2(selectionPlainList[index]);
    if (dataBoxChkCategory2(selectionPlainList[index]) <= 0)
    {
        for (i = index + 1; i < selectionPlainCount; i++)
        {
            selectionPlainList[i - 1] = selectionPlainList[i];
        }
        selectionPlainCount--;
        return 0;
    }
    return dataBoxChkCategory2(selectionPlainList[index]);
}

/* tu101.c's LookupRecord. */
struct LookupRecord {
    char *text;                         /* 0x00: the record's text */
    /* In the special-attack records selectionCopilotRecordCount and selectionCopilotRecordAt scan,
     * attributes[1] is the copilot member mask (bit n for member id n + 1) and attributes[2] the bit
     * of the caller's mask that enables the record; in the pair records of selectionPairRecordCount,
     * attributes[0] and attributes[1] are the two members who perform it. */
    unsigned char attributes[3];
    unsigned char target;               /* 0x07 */
    unsigned char zone;                 /* 0x08 */
    unsigned char type;                 /* 0x09 */
    unsigned char range;                /* 0x0A */
    unsigned char condition;            /* 0x0B */
    unsigned char effect;               /* 0x0C */
    unsigned char unmodeled_0d;
    unsigned short elementMask;         /* 0x0E */
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
        record->text = (char *) txdTableEntry(menuMainTxdBuffer, key);
        record += 1;
    }
    while (index < 99);
}

extern unsigned char D_003EC0D7[];
/* OV01 battleSlotMarkTest (config/symbols/ov01.txt); main reads its result. */
int battleSlotMarkTest(int key, int mark);

/* The D_003EC0D7 value of character id; for id 2 it is 3 higher, and 4 higher while its mark 4 is set. */
int selectionMemberCapacity(int id)
{
    int value;

    value = D_003EC0D7[id];
    if (id == 2)
    {
        value += 3;
        if (battleSlotMarkTest(id, 4) != 0)
        {
            value += 1;
        }
    }
    return value;
}

int ptFindSlot(int id);

/* Counts the pair records of D_003EBAA8 (SELECTION_PAIR_RECORD_FIRST, 16 of them) that are enabled in
   mask (bit attributes[2]), have member as one of their two members and whose other member ptFindSlot
   finds. */
int selectionPairRecordCount(int member, unsigned short mask)
{
    int count;
    int i;

    count = 0;
    for (i = SELECTION_PAIR_RECORD_FIRST; i < SELECTION_PAIR_RECORD_FIRST + SELECTION_PAIR_RECORD_COUNT; i++)
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
int selectionPairRecordAt(int member, int nth, unsigned short mask)
{
    int found;
    int i;

    found = 0;
    for (i = 0; i < SELECTION_PAIR_RECORD_COUNT; i++)
    {
        if ((mask >> D_003EBAA8[i + SELECTION_PAIR_RECORD_FIRST].attributes[2]) & 1)
        {
            if (D_003EBAA8[i + SELECTION_PAIR_RECORD_FIRST].attributes[0] == member)
            {
                if (ptFindSlot(D_003EBAA8[i + SELECTION_PAIR_RECORD_FIRST].attributes[1]) != -1)
                {
                    found++;
                    if (nth + 1 == found)
                    {
                        break;
                    }
                }
            }
            else if (D_003EBAA8[i + SELECTION_PAIR_RECORD_FIRST].attributes[1] == member)
            {
                if (ptFindSlot(D_003EBAA8[i + SELECTION_PAIR_RECORD_FIRST].attributes[0]) != -1)
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
    if (i == SELECTION_PAIR_RECORD_COUNT)
    {
        return -1;
    }
    return (i & (SELECTION_PAIR_RECORD_COUNT - 1)) + SELECTION_PAIR_RECORD_FIRST;
}

SelectionHolder *ptFindRecord(int id);

/* Counts the records among member's seven (from D_003EBAA8[member * 10 - 58]) that are enabled in mask
   (bit attributes[2]) and whose copilot member mask includes a member ptFindRecord finds. */
int selectionCopilotRecordCount(int member, unsigned short mask)
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
int selectionCopilotRecordAt(int member, int nth, unsigned short mask)
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
char *selectionRecordSetValue(int record, int value)
{
    SelectionRecord *entry;

    entry = &selectionRecordTable[record];
    if (entry->kind < 2)
    {
        return func_00A9B850(entry->dispatch_value, value);
    }
    else
    {
        return func_00A9B750(entry->dispatch_value, value);
    }
}

char *selectionSetValue(int group, int slot, int sub, int value)
{
    return selectionRecordSetValue(selectionRecordIndex(group, slot, sub), value);
}

unsigned short selectionGetValue(int group, int slot, int sub)
{
    return selectionRecordTable[selectionRecordIndex(group, slot, sub)].value;
}

/* Group 3 has only four slots: its slots 4..7 count as taken. */
int selectionSubIsTaken(SelectionHolder *holder, int group, int slot, int sub)
{
    if (group == 3 && slot >= 4)
    {
        return 1;
    }
    return ((unsigned int) holder->bits->cells[group][slot] >> sub) & 1;
}

/* OV01 battleSlotCodeTest (config/symbols/ov01.txt); main reads its result. */
int battleSlotCodeTest(int key, int code);

/* Whether holder has entry index: the taken bit of a selectionRecordIndex value, or for the extra entries
   selectionCollectEntries appends, the battle-slot flag or code it stands for. */
int selectionHolderHasEntry(SelectionHolder *holder, int index)
{
    if (index == SELECTION_ENTRY_NONE)
    {
        return 0;
    }
    if (index == SELECTION_ENTRY_EXTRA_FLAG)
    {
        return battleSlotFlagTest(holder->id, SELECTION_EXTRA_FLAG) != 0;
    }
    if (index > SELECTION_ENTRY_EXTRA_FLAG)
    {
        return battleSlotCodeTest(holder->id, index - SELECTION_EXTRA_CODE_ENTRY_OFFSET) != 0;
    }
    return ((unsigned int) holder->bits->cells[index >> 5][(index >> 2) & 7] >> (index & 3)) & 1;
}

/* The number of taken subs of (group, slot); the bits are summed pairwise, as in the original. */
int selectionEntryBitCount(SelectionHolder *holder, int group, int slot)
{
    int sub0;
    int sub3;
    int sub2;

    sub0 = selectionSubIsTaken(holder, group, slot, 0);
    sub3 = selectionSubIsTaken(holder, group, slot, 3);
    sub2 = selectionSubIsTaken(holder, group, slot, 2);
    sub0 += sub3;
    sub2 += selectionSubIsTaken(holder, group, slot, 1);
    return sub0 + sub2;
}

/* The number of taken subs of group: the counts of its eight slots, summed pairwise as in the original. */
int selectionRowBitCount(SelectionHolder *holder, int group)
{
    int slot0;
    int slot1;
    int slot2;
    int slot3;
    int slot4;
    int slot5;
    int slot6;
    int slot7;

    slot0 = selectionEntryBitCount(holder, group, 0);
    slot7 = selectionEntryBitCount(holder, group, 7);
    slot3 = selectionEntryBitCount(holder, group, 3);
    slot5 = selectionEntryBitCount(holder, group, 5);
    slot1 = selectionEntryBitCount(holder, group, 1);
    slot2 = selectionEntryBitCount(holder, group, 2);
    slot4 = selectionEntryBitCount(holder, group, 4);
    slot6 = selectionEntryBitCount(holder, group, 6);
    slot3 += slot5;
    slot4 += slot6;
    slot0 += slot7;
    slot1 += slot2;
    slot0 += slot3;
    slot1 += slot4;
    return slot0 + slot1;
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
   taken or not), then the extra entries it holds (SELECTION_ENTRY_EXTRA_FLAG for kind 1; 0x71..0x76 for
   kind 2 of the character with id 3), and returns the count. */
int selectionCollectEntries(unsigned char *list, SelectionHolder *holder, int kind)
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
                if (kind == 3 || selectionSubIsTaken(holder, group, slot, sub) != 0)
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
    if (battleSlotFlagTest(holder->id, SELECTION_EXTRA_FLAG) != 0 && kind == 1)
    {
        list[count++] = SELECTION_ENTRY_EXTRA_FLAG;
    }
    if (kind == 2 && holder->id == 3)
    {
        /* The group counter is reused for the extra codes: the original keeps them in group's
           register, and a separate variable is allocated another one. */
        for (group = SELECTION_EXTRA_CODE_FIRST; group < SELECTION_EXTRA_CODE_END; group++)
        {
            if (battleSlotCodeTest(holder->id, group) != 0)
            {
                list[count++] = group + SELECTION_EXTRA_CODE_ENTRY_OFFSET;
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
   selectionSubIsTaken reports for its missing slots 4..7 already reach. */
int selectionGroupIsComplete(SelectionHolder *holder, int group)
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

/* Whether group can open slot: not open yet, group open, and its D_003EC0E8 cost within classPoints. */
int selectionCanOpenSlot(SelectionHolder *holder, int group, int slot)
{
    if (((holder->bits->header[group] >> slot) & 1) || holder->bits->openGroup < group
        || holder->counter->classPoints < D_003EC0E8[group * 8 + slot])
    {
        return 0;
    }
    return 1;
}

/* Whether sub of (group, slot) can be taken: not taken yet, slot open, and its record value within skillPoints. */
int selectionCanTakeSub(SelectionHolder *holder, int group, int slot, int sub)
{
    if (((holder->bits->cells[group][slot] >> sub) & 1) || !((holder->bits->header[group] >> slot) & 1)
        || holder->counter->skillPoints < selectionRecordTable[selectionRecordIndex(group, slot, sub)].value)
    {
        return 0;
    }
    return 1;
}

/* Takes sub of (group, slot) and pays its record value in skill points; returns 1 once all four subs
   of the slot are taken, opening the next group when the slot belongs to the highest open one. */
int selectionTakeSub(SelectionHolder *holder, int group, int slot, int sub)
{
    int index;
    SelectionCounter *counter;
    int open;

    holder->bits->cells[group][slot] |= 1 << sub;
    index = selectionRecordIndex(group, slot, sub);
    counter = holder->counter;
    counter->skillPoints -= selectionRecordTable[index].value;
    holder->bits->takenCount++;
    if (holder->bits->cells[group][slot] != SELECTION_SLOT_ALL_SUBS)
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

/* OV01 battleSlotFlagSet and battleSlotCodeSet (config/symbols/ov01.txt). */
void battleSlotFlagSet(int key, unsigned char flag);
void battleSlotCodeSet(int key, unsigned char code);

/* Applies the battle-slot flag or code of the record of (group, slot, sub) to holder's character
   and takes the sub (selectionTakeSub). */
int selectionApplyEntry(SelectionHolder *holder, int group, int slot, int sub)
{
    int index;
    SelectionRecord *entry;

    index = selectionRecordIndex(group, slot, sub);
    entry = &selectionRecordTable[index];
    if (entry->kind == 2)
    {
        battleSlotCodeSet(holder->id, entry->dispatch_value);
    }
    else
    {
        battleSlotFlagSet(holder->id, entry->dispatch_value);
    }
    return selectionTakeSub(holder, group, slot, sub);
}

/* Opens slot bit of group slot and pays its D_003EC0E8 cost in class points. */
void selectionHolderCharge(SelectionHolder *holder, int slot, int bit)
{
    SelectionCounter *counter;

    holder->bits->header[slot] |= 1 << bit;
    counter = holder->counter;
    counter->classPoints -= D_003EC0E8[slot * 8 + bit];
}

/* The clock reading xglClockRead fills (xgl_cd.c's XglClock). */
typedef struct XglClock {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

/* The ten holders' buffers (ptMemberRecords[i].bits); tu108.c and tu110.c view them as SaveMemberName. */
extern struct ControlBuffer D_006A5880[];
extern SelectionHolder ptMemberRecords[];
/* The block ptState.block points at. */
extern SelectionBlock D_006A5A30;
/* OV01 battleSlotsInit and dataBoxIncCategory3 (config/symbols/ov01.txt). */
void battleSlotsInit(void);
void dataBoxIncCategory3(int id);
signed char saveTextMaximaCopy(signed char *maxima);
void saveTextStateReset(void);
void xglClockRead(XglClock *time);
unsigned int xglClockDayTime2UInt(XglClock *time);

/* Clears the ten holders' taken bits and gives them SELECTION_START_POINTS skill and class points,
   takes the first two subs of (0, 0) for the member with id 4, resets the selection state, adds
   SELECTION_POINTS_PER_MAXIMUM skill points per saved maximum when saveTextMaximaCopy reports them,
   fills the eligibility bitmap and stamps the play time.
   Two spellings per object are needed: the clear loop stores to D_006A5880[member] directly while the
   id-4 grant reloads ptMemberRecords[member].bits for each update, and mapFlag and progressId are
   stored to D_006A5A30 directly while id and playTime go through ptState.block, as in the original. */
void selectionInit(void)
{
    signed char maxima[6];
    XglClock now;
    int member;
    int group;
    int slot;
    SelectionRecord *entry;

    battleSlotsInit();
    dataBoxClearAll();
    sdvInitSpecialWork();
    member = 0;
    do
    {
        for (group = 0; group < 4; group++)
        {
            D_006A5880[member].header[group] = 0;
            for (slot = 0; slot < 8; slot++)
            {
                D_006A5880[member].cells[group][slot] = 0;
            }
            D_006A5880[member].entries[group] = 0;
        }
        D_006A5880[member].openGroup = 0;
        D_006A5880[member].takenCount = 0;
        D_006A5880[member].entry_count = 0;
        ptMemberRecords[member].counter->skillPoints = SELECTION_START_POINTS;
        ptMemberRecords[member].counter->classPoints = SELECTION_START_POINTS;
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
                battleSlotCodeSet(4, entry->dispatch_value);
            }
            else
            {
                battleSlotFlagSet(4, entry->dispatch_value);
            }
            entry = &selectionRecordTable[selectionRecordIndex(0, 0, 1)];
            if (entry->kind == 2)
            {
                battleSlotCodeSet(4, entry->dispatch_value);
            }
            else
            {
                battleSlotFlagSet(4, entry->dispatch_value);
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
        ptMemberRecords[0].counter->skillPoints += maxima[0] * SELECTION_POINTS_PER_MAXIMUM;
        ptMemberRecords[1].counter->skillPoints += maxima[1] * SELECTION_POINTS_PER_MAXIMUM;
        ptMemberRecords[2].counter->skillPoints += maxima[2] * SELECTION_POINTS_PER_MAXIMUM;
        ptMemberRecords[4].counter->skillPoints += maxima[3] * SELECTION_POINTS_PER_MAXIMUM;
        ptMemberRecords[5].counter->skillPoints += maxima[4] * SELECTION_POINTS_PER_MAXIMUM;
        ptMemberRecords[6].counter->skillPoints += maxima[5] * SELECTION_POINTS_PER_MAXIMUM;
        battleSlotFlagSet(5, SELECTION_EXTRA_FLAG);
        battleSlotFlagSet(2, SELECTION_EXTRA_FLAG);
        dataBoxIncCategory3(0x45);
    }
    saveTextStateReset();
    selectionBlockFill();
    xglClockRead(&now);
    ptState.block->playTime = xglClockDayTime2UInt(&now);
}
