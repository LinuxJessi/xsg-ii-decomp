#include "common.h"
#include "shared.h"
#include "main/tu048.h"
#include "main/tu124.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "main/tu051.h"
#include "ov01/tu010.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"
#include "ov01/tu036.h"
#include "ov01/tu042.h"
#include "ov01/tu043.h"


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

/* The unit's actor and weapon slots point at XenoActors (main/tu051.h). */

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
    struct XenoActor *work;       /* +0x10 */
    struct XenoActor *actor;      /* +0x14 */
    unsigned char unmodeled_18[0x1C - 0x18];
    struct XenoActor *weapon[4];  /* +0x1C */
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
int choiceMenuGetResult(int);
extern int D_00AC3C04;
extern s16 choiceMenuEntryIds[];
extern u8 choiceMenuEntryFlags[];
extern char *D_00AC4160;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3C04, 0 when choiceMenuEntryFlags flags it. */
int func_00A97F40(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C04 = entry_id;
            result = hitActionTextGet(entry_id, 1);
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

extern int D_00AC3C08;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3C08, 0 when choiceMenuEntryFlags flags it. */
int func_00A981B8(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C08 = entry_id;
            result = battleLookupTextGet(entry_id, 1);
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

extern int D_00AC3C0C;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3C0C, 0 when choiceMenuEntryFlags flags it. */
int func_00A98440(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C0C = entry_id;
            result = calcLevelTextGet(entry_id, 1);
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

extern int D_00AC3C30;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3C30, 0 when choiceMenuEntryFlags flags it. */
int func_00A986C8(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C30 = entry_id;
            result = pairLinkTextGet(entry_id, 1);
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
/* The enemy cursor's text page list for textEntryAlloc: the text (set by
 * func_00A98960), then 0. */
extern int enemyCursorPages[2];

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3C80, 0 when choiceMenuEntryFlags flags it;
 * the helper result goes to enemyCursorPages instead of D_00AC4160, the word the other pickers use. */
int func_00A98960(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3C80 = entry_id;
            result = battleLookupTextGet(entry_id, 1);
            enemyCursorPages[0] = result;
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

extern int D_00AC3CAC;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in D_00AC3CAC, 0 when choiceMenuEntryFlags flags it. */
int func_00A98BE8(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index];
            D_00AC3CAC = entry_id;
            result = tgtIdTextGet(entry_id, 1);
            D_00AC4160 = result;
            if (choiceMenuEntryFlags[cursor_index] != 0) {
                D_00AC3CAC = 0;
            }
        }
    }
    return D_00AC3CAC;
}

extern int statBitValues[];

/* Rows of status bits of a unit (StatBlock.activeMask; statBitValueGet indexes them). */
#define STAT_BIT_ROWS 4

/* The value of status bit mask of row: -1 when the bit is not active or has no value. */
int statBitValueGet(Ov01Object *unit, int row, int mask) {
    int *values;
    int index;
    int value;

    if ((unit->stats->activeMask[row] & mask) == 0) {
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

/* Enemy cursor text entry: position, size and style passed to textEntryAlloc. */
#define ENEMY_CURSOR_X 10
#define ENEMY_CURSOR_Y 0x58
#define ENEMY_CURSOR_WIDTH 0x90
#define ENEMY_CURSOR_HEIGHT 0x14
#define ENEMY_CURSOR_STYLE 7

ObjectTask *textEntryAlloc(int x, int y, int width, int height, int style, void *pages);
/* The status shown by the enemy cursor: row and bit mask (enemyCursorStep walks them with statBitValueGet) and
 * the frames left before the next one. */
extern int enemyCursorRow;
extern int enemyCursorBit;
extern int enemyCursorTimer;

/* Opens the enemy cursor's text entry when the unit has any status bit set. */
void enemyCursorOpen(Ov01Object *unit)
{
    u16 *bits = unit->stats->activeMask;
    int row;

    for (row = 0; row < STAT_BIT_ROWS; row++) {
        if (bits[row] != 0) {
            break;
        }
    }
    if (row < STAT_BIT_ROWS) {
        enemyCursorPages[1] = 0;
        enemyCursorPages[0] = 0;
        pTaskEnCur = textEntryAlloc(ENEMY_CURSOR_X, ENEMY_CURSOR_Y, ENEMY_CURSOR_WIDTH, ENEMY_CURSOR_HEIGHT,
                                    ENEMY_CURSOR_STYLE, enemyCursorPages);
        enemyCursorTimer = 0;
        enemyCursorRow = 0;
        enemyCursorBit = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", enemyCursorStep);

void objCurRemove(void)
{
    if (pTaskEnCur != 0) {
        objRemove(pTaskEnCur);
        pTaskEnCur = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", battleSelectStep);

/* Menu selections of the battle menu (BattleState.selection) and the commands (CmdListBody.command) they give. */
enum {
    SELECT_TARGET_LAST = 10,                /* 1..10: key-row entries (tableKeyEntryGet) */
    SELECT_WAIT = 11,
    SELECT_LOOKUP,
    SELECT_HIT_ACTION,
    SELECT_PAIR,
    SELECT_INDEX_6,
    SELECT_INDEX_5,
    SELECT_LEVEL,
    SELECT_COMMAND_7,
    SELECT_TGT_ID,
    SELECT_END = 20
};

/* The command of the battle menu's selection for the unit: key-row entries 1..10 (entries 1 and 2 are no command
 * for party keys 1..10), the record-based selections with their technique and record index, and the end of the
 * turn (command 8 for a party unit with selection kind 1 or 2, else CMD_KIND_THINK_END). Returns 1. */
int battleSelectionGet(Ov01Object *unit, CmdListBody *command, Ov01Object *target)
{
    int selection;
    short code;
    BattleLookupEntry *lookup;
    HitAction *hitAction;
    CalcLevelRecord *level;
    PairLink *pair;
    TgtIdRecord *tgtId;

    selection = tgtSelectState.selection;
    if (selection == 0) {
    } else if (selection == SELECT_END) {
        if ((unsigned int)(unit->stats->keyUnsigned - 1) < 10
            && (unsigned int)(tgtSelectState.selectionKind - 1) < 2) {
            command->command = 8;
            tgtSelectState.selectionKind = 0;
            tgtSelectState.selection = 0;
        } else {
            command->command = CMD_KIND_THINK_END;
        }
    } else if (selection <= SELECT_TARGET_LAST) {
        if ((unsigned int)(unit->stats->keyUnsigned - 1) < 10 && (unsigned int)(selection - 1) < 2) {
            command->command = 0;
        } else {
            code = tableKeyEntryGet(unit, tgtSelectState.selection, target);
            command->command = 1;
            command->code = code;
            command->arg.target = target;
        }
    } else {
        switch (selection) {
        case SELECT_WAIT:
            command->command = 2;
            break;
        case SELECT_LOOKUP:
            command->command = 3;
            lookup = battleLookupGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = lookup->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_HIT_ACTION:
            command->command = 4;
            hitAction = hitActionGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = hitAction->entry;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_LEVEL:
            command->command = 1;
            level = calcLevelRecordGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = level->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_PAIR:
            command->command = 1;
            pair = pairLinkGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = pair->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_TGT_ID:
            command->command = 1;
            tgtId = tgtIdRecordGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = tgtId->actionIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_INDEX_6:
            command->command = 6;
            command->code = tgtSelectState.selectionIndex;
            break;
        case SELECT_INDEX_5:
            command->command = 5;
            command->code = tgtSelectState.selectionIndex;
            break;
        case SELECT_COMMAND_7:
            command->command = 7;
            break;
        default:
            command->command = 0;
            break;
        }
    }
    return 1;
}

/* Returns the entry of the key row of the unit's table at index (1-based); the target the callers pass is not used. */

s16 tableKeyEntryGet(StatOwner *owner, int index, void *target) {
    unsigned char *record_table = tableKeyRowGet(owner->stats->key);

    s16 *entry = (s16 *)(record_table + (index - 1) * sizeof(s16));

    return *entry;
}

/* The target slot of the menu: targets[1] while a pair menu mode is open and its flag is set, else targets[0]. */
BattleUnit **tgtUnitAdrGet(void)
{
    switch (tgtSelectState.menuMode) {
    case 7:
    case 10:
    case 12:
    case 14:
    case 16:
        if (tgtSelectState.targetFlags != 0 && (tgtSelectState.targetFlags[0] & 1)) {
            return &tgtSelectState.targets[1];
        }
        return &tgtSelectState.targets[0];
    }
    return &tgtSelectState.targets[0];
}

BattleUnit *tgtUnitGet(void)
{
    return *tgtUnitAdrGet();
}

void tgtUnitSet(BattleUnit *unit)
{
    *tgtUnitAdrGet() = unit;
}

struct BattleState *battleStateGet(void) {
    return &tgtSelectState;
}

extern int thinkNo;

int thinkNoGet(void) {
    return thinkNo;
}

void thinkNoSet(int value) {
    thinkNo = value;
}

/*
 * gameWork (main): encounterTarget is the battle's enemy group number (the MONSTER SET debug row of tu072 sets it,
 * monsSetNoGet of tu046 reads it), encounterFlags the cf event bits (XS1 keeps cfEvent in its game loop state;
 * tu039 tests bit 0x20 for the time limit).
 */
short cfMonsSetNoGet(void) {
    return gameWork.encounterTarget;
}

void cfMonsSetNoSet(int monsSetNo) {
    gameWork.encounterTarget = monsSetNo;
}

int cfEventGet(void) {
    return gameWork.encounterFlags;
}

void cfEventSet(int cfEvent) {
    gameWork.encounterFlags = cfEvent;
}

extern unsigned char ov01PartyTable[];

/* Returns the party table ov01PartyTable (0x14-byte records, one per party slot). */
void *ov01PartyListGet(void) {
    return &ov01PartyTable;
}

/* Gattai code word: GATTAI_CODE_BASE plus the gattai id. gattaiCodeGet reads it back. */
extern int gattaiCode;

#define GATTAI_CODE_BASE 0x16000
/* Story flag field (8 bits at flag 0x1EE) holding the current gattai id. */
#define GATTAI_ID_FLAG 0x1EE
#define GATTAI_ID_FLAG_BITS 8

int xglFlagsGet(int bit_offset, int bit_count);

/* The gattai code: from the story flag when a gattai id is set there,
 * else the code gattaiCodeSet stored. */
int gattaiCodeGet(void) {
    int id = xglFlagsGet(GATTAI_ID_FLAG, GATTAI_ID_FLAG_BITS);
    int code;

    if (id == 0) {
        code = gattaiCode;
    } else {
        code = id + GATTAI_CODE_BASE;
    }
    return code;
}

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
