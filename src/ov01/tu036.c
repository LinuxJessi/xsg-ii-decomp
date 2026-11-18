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



/*
 * Private battle state, in original BSS address order. Keeping these local
 * definitions together lets the guarded overlay linker map each exact object
 * to its original storage span.
 */
/* The enemy-cursor task: enemyCursorOpen opens it, enemyCursorClose and objCurRemove
 * remove it. X1 names the same global the enemy-cursor task (battle_init.c). */
extern TextEntry *pTaskEnCur;
/* Text task that the choice-menu openers allocate (the textEntryAlloc result) and the close functions remove. */
extern TextEntry *choiceMenuTextTask;
/* The choice menu's text page list for textEntryAlloc: the text of the entry under the cursor (set by the pickers),
 * then 0. */
extern unsigned char *choiceMenuPages[2];
/* The text page list of the enemy cursor (and of the paired technique menu): the text (set by pairedMenuPick and
 * enemyCursorStep), then 0. */
extern unsigned char *enemyCursorPages[2];

/* The lists the choice-menu openers fill for choice slot 0: the entry texts (0 after the last), the value shown with
 * each entry, its id and its flag (CHOICE_ENTRY_DISABLED greys it out). The pickers index ids and flags with the
 * 1-based cursor. */
extern unsigned char *choiceMenuItems[];
extern short choiceMenuValues[];
extern short choiceMenuEntryIds[];
extern unsigned char choiceMenuEntryFlags[];

#define CHOICE_ENTRY_DISABLED 3
/* Layout of the openers' choice menu (slot 0) and of their text entry below it; each opener has its own style
 * index and unit size. */
#define CHOICE_MENU_X 0x14
#define CHOICE_MENU_Y 0x6C
#define CHOICE_MENU_WIDTH 0x1AC
#define CHOICE_MENU_HEIGHT 0x70
#define CHOICE_TEXT_X 0x14
#define CHOICE_TEXT_Y 0x118
#define CHOICE_TEXT_WIDTH 0x1AC
#define CHOICE_TEXT_HEIGHT 0x28
#define CHOICE_TEXT_STYLE 8

/* Items 1..ITEM_ID_LAST; an item can be picked in battle when its HitAction flags have ITEM_BATTLE_USE. */
#define ITEM_ID_LAST 40
#define ITEM_BATTLE_USE 0x4000
#define ITEM_MENU_STYLE 2
#define ITEM_MENU_UNIT_SIZE 2

/*
 * Item menu: lists the items the data box holds (dataBoxChkCategory1) that can be used in battle, with their
 * count, greyed out when unit cannot use them on any target (tgtRecordCanTarget), and opens it.
 */
void choiceMenuOpenItems(Ov01Object *unit)
{
    int id;
    int count;
    int n;

    n = 0;
    for (id = 1; id <= ITEM_ID_LAST; id++) {
        count = dataBoxChkCategory1(id);
        if (count > 0 && (hitActionGet(id)->flags & ITEM_BATTLE_USE)) {
            choiceMenuItems[n] = hitActionTextGet(id, RECORD_TEXT_NAME);
            choiceMenuValues[n] = count;
            choiceMenuEntryIds[n] = id;
            choiceMenuEntryFlags[n] = tgtRecordCanTarget(unit, id) ? 0 : CHOICE_ENTRY_DISABLED;
            n++;
        }
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, ITEM_MENU_STYLE,
                   ITEM_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    choiceMenuPages[1] = 0;
    choiceMenuPages[0] = 0;
    choiceMenuTextTask = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                        CHOICE_TEXT_STYLE, choiceMenuPages);
}


/* Closes choice slot 0 and removes the text task choiceMenuTextTask (the five identical close bodies of this TU). */
void choiceMenuTextClose(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

int choiceMenuIsOpen(int);
int choiceMenuGetResult(int);
extern int itemMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in itemMenuChoice, 0 when choiceMenuEntryFlags
 * flags it. */
int itemMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            itemMenuChoice = entry_id;
            result = hitActionTextGet(entry_id, 1);
            choiceMenuPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                itemMenuChoice = 0;
            }
        }
    }
    return itemMenuChoice;
}

/* Techniques 1..TEC_LOOKUP_LAST of the battle lookup table. */
#define TEC_LOOKUP_LAST 59
#define TEC_MENU_STYLE 1
#define TEC_MENU_UNIT_SIZE 2

/*
 * Technique menu: lists the techniques unit has (slotCodeTest) with their EP cost, greyed out when unit cannot
 * use them now (calcEntryUsable), and opens it.
 */
void choiceMenuOpenTecs(Ov01Object *unit)
{
    int id;
    int n;

    n = 0;
    for (id = 1; id <= TEC_LOOKUP_LAST; id++) {
        if (slotCodeTest(unit, id)) {
            choiceMenuItems[n] = battleLookupTextGet(id, RECORD_TEXT_NAME);
            choiceMenuValues[n] = calcEpCost(unit, battleLookupGet(id)->tecIndex);
            choiceMenuEntryIds[n] = id;
            choiceMenuEntryFlags[n] = calcEntryUsable(unit, id) ? 0 : CHOICE_ENTRY_DISABLED;
            n++;
        }
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, TEC_MENU_STYLE,
                   TEC_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    choiceMenuPages[1] = 0;
    choiceMenuPages[0] = 0;
    choiceMenuTextTask = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                        CHOICE_TEXT_STYLE, choiceMenuPages);
}

void func_00A98180(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

extern int tecMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in tecMenuChoice, 0 when choiceMenuEntryFlags
 * flags it. */
int tecMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            tecMenuChoice = entry_id;
            result = battleLookupTextGet(entry_id, 1);
            choiceMenuPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                tecMenuChoice = 0;
            }
        }
    }
    return tecMenuChoice;
}

/* The level record indices calcKeyBandRecords wrote for the level menu, and their count (each record menu has
 * its own pair). */
extern int levelMenuIds[];
extern int levelMenuCount;

#define LEVEL_MENU_STYLE 4
#define LEVEL_MENU_UNIT_SIZE 1

/*
 * Level record menu: lists the level records of unit's key band (calcKeyBandRecords) with their required level,
 * greyed out while unit's current value does not reach it (calcCurrentMeetsRecord), and opens it.
 */
void choiceMenuOpenLevels(Ov01Object *unit)
{
    int i;
    int n;

    n = 0;
    levelMenuCount = calcKeyBandRecords(unit, levelMenuIds);
    for (i = 0; i < levelMenuCount; i++) {
        choiceMenuItems[n] = calcLevelTextGet(levelMenuIds[i], RECORD_TEXT_NAME);
        choiceMenuValues[n] = calcLevelRecordGet(levelMenuIds[i])->requiredLevel;
        choiceMenuEntryIds[n] = levelMenuIds[i];
        choiceMenuEntryFlags[n] = calcCurrentMeetsRecord(unit, levelMenuIds[i]) ? 0 : CHOICE_ENTRY_DISABLED;
        n++;
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, LEVEL_MENU_STYLE,
                   LEVEL_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    choiceMenuPages[1] = 0;
    choiceMenuPages[0] = 0;
    choiceMenuTextTask = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                        CHOICE_TEXT_STYLE, choiceMenuPages);
}

void func_00A98408(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

extern int levelMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in levelMenuChoice, 0 when choiceMenuEntryFlags
 * flags it. */
int levelMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            levelMenuChoice = entry_id;
            result = calcLevelTextGet(entry_id, 1);
            choiceMenuPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                levelMenuChoice = 0;
            }
        }
    }
    return levelMenuChoice;
}

extern int pairMenuIds[];
extern int pairMenuCount;

#define PAIR_MENU_STYLE 3
#define PAIR_MENU_UNIT_SIZE 1

/*
 * Pair link menu: lists the pair links of unit's key (calcKeyPairRecords) with their required level, greyed out
 * while the levels of the pair do not reach it (calcLevelsMeetRecord), and opens it.
 */
void choiceMenuOpenPairs(Ov01Object *unit)
{
    int i;
    int n;

    n = 0;
    pairMenuCount = calcKeyPairRecords(unit, pairMenuIds);
    for (i = 0; i < pairMenuCount; i++) {
        choiceMenuItems[n] = pairLinkTextGet(pairMenuIds[i], RECORD_TEXT_NAME);
        choiceMenuValues[n] = pairLinkGet(pairMenuIds[i])->requiredLevel;
        choiceMenuEntryIds[n] = pairMenuIds[i];
        choiceMenuEntryFlags[n] = calcLevelsMeetRecord(unit, pairMenuIds[i]) ? 0 : CHOICE_ENTRY_DISABLED;
        n++;
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, PAIR_MENU_STYLE,
                   PAIR_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    choiceMenuPages[1] = 0;
    choiceMenuPages[0] = 0;
    choiceMenuTextTask = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                        CHOICE_TEXT_STYLE, choiceMenuPages);
}

void func_00A98690(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

extern int pairMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in pairMenuChoice, 0 when choiceMenuEntryFlags
 * flags it. */
int pairMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            pairMenuChoice = entry_id;
            result = pairLinkTextGet(entry_id, 1);
            choiceMenuPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                pairMenuChoice = 0;
            }
        }
    }
    return pairMenuChoice;
}

extern int pairedMenuIds[];
extern int pairedMenuCount;

#define PAIRED_MENU_STYLE 1
#define PAIRED_MENU_UNIT_SIZE 1

/*
 * Paired technique menu (its text entry is the enemy cursor's): lists the techniques paired with the one picked
 * last (tecMenuChoice, calcPairedRecords) with their EP cost, greyed out when unit cannot use them now
 * (calcPairEntryUsable), and opens it.
 */
void choiceMenuOpenPaired(Ov01Object *unit)
{
    int i;
    int n;

    n = 0;
    pairedMenuCount = calcPairedRecords(unit, tecMenuChoice, pairedMenuIds);
    for (i = 0; i < pairedMenuCount; i++) {
        choiceMenuItems[n] = battleLookupTextGet(pairedMenuIds[i], RECORD_TEXT_NAME);
        choiceMenuValues[n] = calcEpCost(unit, battleLookupGet(pairedMenuIds[i])->tecIndex);
        choiceMenuEntryIds[n] = pairedMenuIds[i];
        choiceMenuEntryFlags[n] = calcPairEntryUsable(unit, pairedMenuIds[i]) ? 0 : CHOICE_ENTRY_DISABLED;
        n++;
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, PAIRED_MENU_STYLE,
                   PAIRED_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    enemyCursorPages[1] = 0;
    enemyCursorPages[0] = 0;
    pTaskEnCur = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                CHOICE_TEXT_STYLE, enemyCursorPages);
}

void enemyCursorClose(void) {
    choiceMenuClose(0);
    objRemove(pTaskEnCur);
    pTaskEnCur = 0;
}

extern int pairedMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in pairedMenuChoice, 0 when choiceMenuEntryFlags
 * flags it; the text goes to enemyCursorPages instead of choiceMenuPages, the list the other pickers use. */
int pairedMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            pairedMenuChoice = entry_id;
            result = battleLookupTextGet(entry_id, 1);
            enemyCursorPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                pairedMenuChoice = 0;
            }
        }
    }
    return pairedMenuChoice;
}

extern int tgtIdMenuIds[];
extern int tgtIdMenuCount;

#define TGT_ID_MENU_STYLE 4
#define TGT_ID_MENU_UNIT_SIZE 1

/*
 * Target id menu: lists the ids of unit's set slot flags (slotFlagCollect) with their minimum level, greyed out
 * when unit's side does not accept them (tgtSideAccepts), and opens it.
 */
void choiceMenuOpenTgtIds(Ov01Object *unit)
{
    int i;
    int n;

    n = 0;
    tgtIdMenuCount = slotFlagCollect(unit, tgtIdMenuIds);
    for (i = 0; i < tgtIdMenuCount; i++) {
        choiceMenuItems[n] = tgtIdTextGet(tgtIdMenuIds[i], RECORD_TEXT_NAME);
        choiceMenuValues[n] = tgtIdRecordGet(tgtIdMenuIds[i])->minLevel;
        choiceMenuEntryIds[n] = tgtIdMenuIds[i];
        choiceMenuEntryFlags[n] = tgtSideAccepts(unit, tgtIdMenuIds[i]) ? 0 : CHOICE_ENTRY_DISABLED;
        n++;
    }
    choiceMenuItems[n] = 0;
    choiceMenuSetActive(0, 1);
    choiceMenuOpen(0, CHOICE_MENU_X, CHOICE_MENU_Y, CHOICE_MENU_WIDTH, CHOICE_MENU_HEIGHT, TGT_ID_MENU_STYLE,
                   TGT_ID_MENU_UNIT_SIZE, 0, choiceMenuItems, choiceMenuValues, choiceMenuEntryFlags);
    choiceMenuPages[1] = 0;
    choiceMenuPages[0] = 0;
    choiceMenuTextTask = textEntryAlloc(CHOICE_TEXT_X, CHOICE_TEXT_Y, CHOICE_TEXT_WIDTH, CHOICE_TEXT_HEIGHT,
                                        CHOICE_TEXT_STYLE, choiceMenuPages);
}

void func_00A98BB0(void) {
    choiceMenuClose(0);
    objRemove(choiceMenuTextTask);
    choiceMenuTextTask = 0;
}

extern int tgtIdMenuChoice;

/* Cursor entry of choice slot 0 (choiceMenuGetResult): caches its id in tgtIdMenuChoice, 0 when choiceMenuEntryFlags
 * flags it. */
int tgtIdMenuPick(void) {
    int cursor_index;
    s16 entry_id;
    char *result;

    if (choiceMenuIsOpen(0) == 1) {
        cursor_index = choiceMenuGetResult(0);
        if (cursor_index > 0) {
            entry_id = choiceMenuEntryIds[cursor_index - 1];
            tgtIdMenuChoice = entry_id;
            result = tgtIdTextGet(entry_id, 1);
            choiceMenuPages[0] = result;
            if (choiceMenuEntryFlags[cursor_index - 1] != 0) {
                tgtIdMenuChoice = 0;
            }
        }
    }
    return tgtIdMenuChoice;
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

/* Frames each status stays in the enemy cursor; statBitValueGet's result when the bit is not set. */
#define ENEMY_CURSOR_FRAMES 30
#define STAT_BIT_NONE -1

/*
 * Every ENEMY_CURSOR_FRAMES frames moves the enemy cursor to the next status bit unit has (from the bit after the
 * shown one, over the rows from the shown row on, then the shown row again from bit 1) and shows its text
 * (tableStringGet), or no text when none is set.
 */
void enemyCursorStep(Ov01Object *unit)
{
    int i;
    int row;
    int bit;
    int value;

    if (enemyCursorTimer <= 0) {
        enemyCursorTimer = ENEMY_CURSOR_FRAMES;
        enemyCursorBit <<= 1;
        value = STAT_BIT_NONE;
        for (i = 0; i < STAT_BIT_ROWS; i++) {
            row = (enemyCursorRow + i) % STAT_BIT_ROWS;
            for (bit = enemyCursorBit; bit & 0xFFFF; bit <<= 1) {
                value = statBitValueGet(unit, row, bit);
                if (value != STAT_BIT_NONE) {
                    enemyCursorRow = row;
                    enemyCursorBit = bit;
                    break;
                }
            }
            if (value != STAT_BIT_NONE) {
                break;
            }
            enemyCursorBit = 1;
        }
        if (value == STAT_BIT_NONE) {
            row = enemyCursorRow;
            for (bit = enemyCursorBit; bit & 0xFFFF; bit <<= 1) {
                value = statBitValueGet(unit, row, bit);
                if (value != STAT_BIT_NONE) {
                    enemyCursorRow = row;
                    enemyCursorBit = bit;
                    break;
                }
            }
        }
        if (value != STAT_BIT_NONE) {
            enemyCursorPages[0] = tableStringGet(value, 1);
        } else {
            enemyCursorPages[0] = 0;
        }
    }
    enemyCursorTimer--;
}

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
    SELECT_SNAPSHOT,
    SELECT_SWAP,
    SELECT_LEVEL,
    SELECT_LEAVE,
    SELECT_TGT_ID,
    SELECT_END = 20
};

/* The command of the battle menu's selection for the unit: key-row entries 1..10 (entries 1 and 2 are no command
 * for party keys 1..10), the record-based selections with their technique and record index, and the end of the
 * turn (CMD_KIND_REFRESH for a party unit with selection kind 1 or 2, else CMD_KIND_THINK_END). Returns 1. */
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
            command->command = CMD_KIND_REFRESH;
            tgtSelectState.selectionKind = 0;
            tgtSelectState.selection = 0;
        } else {
            command->command = CMD_KIND_THINK_END;
        }
    } else if (selection <= SELECT_TARGET_LAST) {
        if ((unsigned int)(unit->stats->keyUnsigned - 1) < 10 && (unsigned int)(selection - 1) < 2) {
            command->command = CMD_KIND_NONE;
        } else {
            code = tableKeyEntryGet(unit, tgtSelectState.selection, target);
            command->command = CMD_KIND_TECHNIQUE;
            command->code = code;
            command->arg.target = target;
        }
    } else {
        switch (selection) {
        case SELECT_WAIT:
            command->command = CMD_KIND_WAIT;
            break;
        case SELECT_LOOKUP:
            command->command = CMD_KIND_LOOKUP;
            lookup = battleLookupGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = lookup->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_HIT_ACTION:
            command->command = CMD_KIND_ITEM;
            hitAction = hitActionGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = hitAction->entry;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_LEVEL:
            command->command = CMD_KIND_TECHNIQUE;
            level = calcLevelRecordGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = level->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_PAIR:
            command->command = CMD_KIND_TECHNIQUE;
            pair = pairLinkGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = pair->tecIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_TGT_ID:
            command->command = CMD_KIND_TECHNIQUE;
            tgtId = tgtIdRecordGet(tgtSelectState.selectionIndex);
            command->arg.target = target;
            command->code = tgtId->actionIndex;
            command->pairIndex = tgtSelectState.selectionIndex;
            break;
        case SELECT_SNAPSHOT:
            command->command = CMD_KIND_SNAPSHOT;
            command->code = tgtSelectState.selectionIndex;
            break;
        case SELECT_SWAP:
            command->command = CMD_KIND_SWAP;
            command->code = tgtSelectState.selectionIndex;
            break;
        case SELECT_LEAVE:
            command->command = CMD_KIND_LEAVE;
            break;
        default:
            command->command = CMD_KIND_NONE;
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
Ov01Object **tgtUnitAdrGet(void)
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

Ov01Object *tgtUnitGet(void)
{
    return *tgtUnitAdrGet();
}

void tgtUnitSet(Ov01Object *unit)
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
