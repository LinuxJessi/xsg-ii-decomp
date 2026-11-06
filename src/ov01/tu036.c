#include "common.h"
#include "shared.h"
#include "main/tu124.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "ov01/tu026.h"
#include "ov01/tu032.h"
#include "ov01/tu036.h"
#include "ov01/tu042.h"
#include "ov01/tu043.h"

typedef struct ObjectTask ObjectTask;

typedef struct XglTaskPrefix XglTaskPrefix;

typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

struct ObjectTask {
    XglTaskPrefix task; /* the scheduler header xglTaskEntryNext maintains. */
    void *work; /* objRemove passes this slot to objWorkFree. */
};

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

/*
 * The battle actor object a unit's actor/weapon slots point to. src/main/
 * chr.h, src/main/near_dir.h, src/main/set_motion.h and src/ov01/unit_cmd.h
 * all restate this same 0x70-byte head verbatim (a TU-local header cannot be
 * included from another TU); this TU adds only the words its own code
 * touches: the float at +0xC0 (transWepIn/transWepOut set it to 0.0/1.0, the
 * same "transparency" member src/ov01/unit_cmd.h restates at that offset)
 * and the flags word at +0x6F0 (hairWindOn ORs in HAIR_WIND_BIT there for
 * every other unit's actor and hairWindOff clears it again).
 */
typedef struct Actor {
    u32 flags;
    void (*update)(struct Actor *actor);
    void (*draw)(struct Actor *actor);
    u32 quadword_alignment_gap;
    Vector4 position;
    Vector4 previous_position;
    Vector4 velocity;
    Vector4 acceleration;
    Vector4 rotation;
    Vector4 scale;
    unsigned char unmodeled_70[0xC0 - 0x70];
    float transparency; /* +0xC0 */
    unsigned char unmodeled_c4[0x6F0 - 0xC4];
    u32 windFlags; /* +0x6F0 */
} Actor;

/*
 * The unit record scenarioMtd, transWepIn/transWepOut and spWepDispOn
 * (0x00a0e550) all share: spWepDispOn receives the
 * same pointer scenarioMtd receives, dereferences it to reach this record,
 * then indexes its +0x1C table by (weapon-slot & 3) and reads +0x14, exactly
 * as transWepIn/transWepOut do on their own argument and scenarioMtd does on
 * *unitHandle. Bytes between the evidenced members are untouched here.
 */
typedef struct BattleUnit BattleUnit;

struct BattleUnit {
    unsigned char unmodeled_00[0x10];
    Actor *work;       /* +0x10 */
    Actor *actor;      /* +0x14 */
    unsigned char unmodeled_18[0x1C - 0x18];
    Actor *weapon[4];  /* +0x1C */
};

BattleUnit **tgtUnitAdrGet(void);
BattleUnit *tgtUnitGet(void);
/*
 * Private battle state, in original BSS address order. Keeping these local
 * definitions together lets the guarded overlay linker map each exact object
 * to its original storage span.
 */
/* The enemy-cursor task: enemyCursorOpen opens it, enemyCursorClose and objCurRemove
 * remove it. X1 names the same global the enemy-cursor task (battle_init.c). */
extern ObjectTask *pTaskEnCur;
/*
 * objEntry2 and objStdInit are defined by ov01/tu001 obj.c (src/ov01/obj.h
 * is that TU-local header and is not includable here);
 * src/ov01/entry_first_init.c already redeclares them the same way.
 */
extern ObjectTask *objEntry2(void *argument, void (*callback)(ObjectTask *task));
extern void objRemove(ObjectTask *task);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97D68);

/* Text task that the choice-menu openers allocate (the textEntryAlloc result) and the close functions remove. */
extern ObjectTask *choiceMenuTextTask;

/* Closes choice slot 0 and removes the text task choiceMenuTextTask (the five identical close bodies of this TU). */
void choiceMenuTextClose(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int choiceMenuIsOpen(int);
int func_00235D78(int);
int func_00A9B8D8(s16, int);
extern int D_00AC3C04;
extern s16 choiceMenuEntryIds[];
extern u8 choiceMenuEntryFlags[];
extern int D_00AC4160;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3C04, 0 when choiceMenuEntryFlags flags it. */
int func_00A97F40(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C04 = entry_id;
            result = func_00A9B8D8(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3C04 = 0;
            }
        }
    }
    return D_00AC3C04;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97FE0);

void func_00A98180(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int func_00A9B750(s16, int);
extern int D_00AC3C08;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3C08, 0 when choiceMenuEntryFlags flags it. */
int func_00A981B8(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C08 = entry_id;
            result = func_00A9B750(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3C08 = 0;
            }
        }
    }
    return D_00AC3C08;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98258);

void func_00A98408(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int func_00A9BAB8(s16, int);
extern int D_00AC3C0C;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3C0C, 0 when choiceMenuEntryFlags flags it. */
int func_00A98440(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C0C = entry_id;
            result = func_00A9BAB8(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3C0C = 0;
            }
        }
    }
    return D_00AC3C0C;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A984E0);

void func_00A98690(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int func_00A9BA40(s16, int);
extern int D_00AC3C30;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3C30, 0 when choiceMenuEntryFlags flags it. */
int func_00A986C8(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C30 = entry_id;
            result = func_00A9BA40(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3C30 = 0;
            }
        }
    }
    return D_00AC3C30;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98768);

void enemyCursorClose(void) {
    choiceMenuClose(0);
    objRemove(pTaskEnCur);
    pTaskEnCur = 0;
}

extern int D_00AC3C80;
extern int D_00AC4170;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3C80, 0 when choiceMenuEntryFlags flags it;
 * the helper result goes to D_00AC4170 instead of D_00AC4160, the word the other pickers use. */
int func_00A98960(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C80 = entry_id;
            result = func_00A9B750(entry_id, 1);
            D_00AC4170 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3C80 = 0;
            }
        }
    }
    return D_00AC3C80;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98A00);

void func_00A98BB0(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int func_00A9BB30(s16, int);
extern int D_00AC3CAC;

/* Cursor entry of choice slot 0 (func_00235D78): caches its id in D_00AC3CAC, 0 when choiceMenuEntryFlags flags it. */
int func_00A98BE8(void) {
    int cursor_index;
    s16 entry_id;
    int result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = func_00235D78(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3CAC = entry_id;
            result = func_00A9BB30(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3CAC = 0;
            }
        }
    }
    return D_00AC3CAC;
}

extern int statBitValues[];

/*
 * The owner's +0x144 pointer names a table whose halfword at +0x92 is
 * indexed by the row argument. bits[1] models only the element this
 * function reads.
 */
typedef struct StatBitTable StatBitTable;

typedef struct StatBitOwner StatBitOwner;

struct StatBitTable {
    unsigned char unmodeled_00[0x92];
    u16 bits[1];
};

struct StatBitOwner {
    unsigned char unmodeled_00[0x144];
    StatBitTable *table; /* +0x144 */
};

int statBitValueGet(StatBitOwner *owner, int row, int mask) {
    int *values;
    int index;
    int value;

    if ((owner->table->bits[row] & mask) == 0) {
        return -1;
    }
    values = statBitValues;
    index = calcStatBit2Idx(mask);
    value = values[row * 16 + index];
    if (value != 0) {
        return value;
    }
    return -1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", enemyCursorOpen);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", enemyCursorStep);

void objCurRemove(void)
{
    if (pTaskEnCur != 0) {
        objRemove(pTaskEnCur);
        pTaskEnCur = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", battleSelectStep);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", battleSelectionGet);

/* Only the members tableKeyEntryGet reads are modeled; the rest of the block is
 * left unmodeled. */
typedef struct TableKey TableKey;

struct TableKey {
    unsigned char unmodeled_00[0x62];
    s16 table_id; /* +0x62: the id tableKeyRowGet maps to a table */
};

typedef struct TableKeyOwner TableKeyOwner;

struct TableKeyOwner {
    unsigned char unmodeled_00[0x144];
    TableKey *key; /* +0x144 */
};

/* Returns the record table for the key: the caller adds the byte offset itself. */

s16 tableKeyEntryGet(TableKeyOwner *owner, int index) {
    unsigned char *record_table = tableKeyRowGet(owner->key->table_id);

    s16 *entry = (s16 *)(record_table + (index - 1) * sizeof(s16));

    return *entry;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", tgtUnitAdrGet);

BattleUnit *tgtUnitGet(void)
{
    return *tgtUnitAdrGet();
}

void tgtUnitSet(BattleUnit *unit)
{
    *tgtUnitAdrGet() = unit;
}

extern unsigned char tgtSelectState[];

void *battleStateGet(void) {
    return &tgtSelectState;
}

extern int thinkNo;

int thinkNoGet(void) {
    return thinkNo;
}

void thinkNoSet(int value) {
    thinkNo = value;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DC8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", cfEventGet);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", cfEventSet);

extern unsigned char ov01PartyTable[];

/* Returns the party table ov01PartyTable (0x14-byte records, one per party slot). */
void *ov01PartyListGet(void) {
    return &ov01PartyTable;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99E18);

/* Gattai code word: 0x16000 plus the gattai id. func_00A99E18 reads it back. */
extern int gattaiCode;

void gattaiCodeSet(int code) {
    gattaiCode = code;
}

void battleSlotsInit(void);
void sbInit(void);
void configInit(void);

/* Resets every ov01 table and initializes the subsystems, then defaults the gattai code to 0x16001. */
void ov01InitAll(void) {
    ov01ResetAll();
    sbInit();
    configInit();
    dataBatDatLoad();
    battleSlotsInit();
    dataBoxClearAll();
    sdvInitSpecialWork();
    gattaiCodeSet(0x16001);
}
