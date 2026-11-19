#ifndef INCLUDE_OV01_TU036_H
#define INCLUDE_OV01_TU036_H

/* Functions of src/ov01/tu036.c that other files call. */

int cfEventGet(void);

/* Bits of the cfEvent word (cfEventGet). */
#define CF_EVENT_BOOST_BONUS 0x1                /* battleStartBonusApply: 1..2 boost stocks for side 0 */
#define CF_EVENT_PARTY_BONUS 0x2                /* battleStartBonusApply: current or levels for the party */
#define CF_EVENT_BONUSES (CF_EVENT_BOOST_BONUS | CF_EVENT_PARTY_BONUS)
#define CF_EVENT_TIME_LIMIT 0x20                /* the battle has a time limit (gameWork.panelValue) */
#define CF_EVENT_SKIP_VICTORY 0x200             /* the battle ends without the victory sequence */

void cfEventSet(int cfEvent);
int cfMonsSetNoGet(void);
void cfMonsSetNoSet(int monsSetNo);
int gattaiCodeGet(void);
struct Ov01Object;

/*
 * The battle state block at tgtSelectState: the target selection of tu035 and the battle values the other files
 * read through battleStateGet. flags holds the stage bits 1 (begin), 2 (activate), 4 (cursor) and 8 (execute)
 * that tgtSelectStep dispatches on. The words at +0x04, +0x30 and +0x54 also have scaffold labels
 * (tgtSelectCurrent, tgtSelectStatus, tgtSelectMode); three functions of tu035 reach them through those labels.
 */
typedef struct BattleState BattleState;

struct BattleState {
    int flags;                              /* +0x00: the stage bits */
    struct Ov01Object *current;             /* +0x04: the object under the cursor */
    struct Ov01Object *lastObject;          /* +0x08: the object tgtSelectConfirm picked */
    struct Ov01Object *targets[2];          /* +0x0C: the target slots tgtUnitAdrGet hands out */
    struct Ov01Object *lastPartyUnit;       /* +0x14: the last party unit whose turn ended */
    int menuMode;                           /* +0x18: BATTLE_MENU_*; BATTLE_MENU_KEYS until tgtSelectManual opens the
                                             * target menu; the *_TARGET modes can use targets[1] */
    int phase;                              /* +0x1C */
    int selection;                          /* +0x20: battleSelectStep's choice (0 none, TGT_SELECTION_BACK) */
    int selectionKind;                      /* +0x24: set by battleSelectStep; battleSelectionGet takes 1 and 2 */
    int active;                             /* +0x28: battleSelectStep sets 2 when the command panel opens, 1 when
                                             * it is closed */
    int selectionIndex;                     /* +0x2C: the record the menu selection picked (battleSelectionGet) */
    int status;                             /* +0x30: TGT_STATUS_* (tu035) */
    int victoryStarted;                     /* +0x34: set once battleEndCheck has started the victory */
    struct Ov01Object *object;              /* +0x38: cleared at the end of a turn */
    int commandKind;                        /* +0x3C: the CMD_KIND_* of the command eventUnitCommand queued for object
                                             * (kinds 1..4 and 6) */
    int hitDamage;                          /* +0x40: the damage of attackQueue's last hit (HitRecord.damage) */
    int hitMode;                            /* +0x44: its hit mode word (HitRecord.hitMode); battleResolveStatus passes
                                             * object, commandKind, hitDamage and hitMode on */
    int transition;                         /* +0x48: 1 when the motion mode is below 16 (a party unit left), else 2 */
    int sideToggle;                         /* +0x4C: flipped by L1/L2 in the cursor stage */
    int moveCmd;                            /* +0x50: set by setMoveCmd */
    int mode;                               /* +0x54 */
    int slotIndex;                          /* +0x58: the index battleEntryActivate and unitRecreate use */
    float scale;                            /* +0x5C: the angle unitRecreate restores */
    unsigned char *targetFlags;             /* +0x60: bit 0 of its first byte selects targets[1] */
};

/* Modes of the battle menu (BattleState.menuMode) that battleSelectStep steps; the *_TARGET modes pick the target of
 * the chosen record (tgtUnitAdrGet). */
enum BattleMenuMode {
    BATTLE_MENU_KEYS,                       /* key input (dataKeyInputStep), PAD_CROSS opens the command panel */
    BATTLE_MENU_KEY_HELD,                   /* a key was entered: waits for DATA_KEY_CANCEL */
    BATTLE_MENU_PANEL_OPEN,
    BATTLE_MENU_PANEL,
    BATTLE_MENU_END,                        /* a choice was made: back to BATTLE_MENU_KEYS with SELECT_END */
    BATTLE_MENU_WAIT,
    BATTLE_MENU_TECS,
    BATTLE_MENU_TEC_TARGET,
    BATTLE_MENU_PAIRED,
    BATTLE_MENU_ITEMS,
    BATTLE_MENU_ITEM_TARGET,
    BATTLE_MENU_LEVELS,
    BATTLE_MENU_LEVEL_TARGET,
    BATTLE_MENU_TGT_IDS,
    BATTLE_MENU_TGT_ID_TARGET,
    BATTLE_MENU_PAIRS,
    BATTLE_MENU_PAIR_TARGET,
    BATTLE_MENU_IDLE,                       /* battleSelectStep does nothing in this mode */
    BATTLE_MENU_SNAPSHOT,
    BATTLE_MENU_SWAP,
    BATTLE_MENU_LEAVE
};

extern BattleState tgtSelectState;

void battleSelectStep(struct Ov01Object *unit);

struct BattleState *battleStateGet(void);
struct CmdListBody;
int battleSelectionGet(struct Ov01Object *unit, struct CmdListBody *command, struct Ov01Object *target);
struct Ov01Object **tgtUnitAdrGet(void);
struct Ov01Object *tgtUnitGet(void);
void tgtUnitSet(struct Ov01Object *unit);

short tableKeyEntryGet(struct Ov01Object *owner, int index, void *target);
void gattaiCodeSet(int code);
void thinkNoSet(int value);

#endif /* INCLUDE_OV01_TU036_H */
