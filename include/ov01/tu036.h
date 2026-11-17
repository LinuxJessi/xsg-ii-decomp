#ifndef INCLUDE_OV01_TU036_H
#define INCLUDE_OV01_TU036_H

/* Functions of src/ov01/tu036.c that other files call. */

int cfEventGet(void);
void cfEventSet(int cfEvent);
short cfMonsSetNoGet(void);
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
    struct Ov01Object *last_object;         /* +0x08: the object tgtSelectConfirm picked */
    struct BattleUnit *targets[2];          /* +0x0C: the target slots tgtUnitAdrGet hands out */
    struct Ov01Object *lastPartyUnit;       /* +0x14: the last party unit whose turn ended */
    int menuMode;                           /* +0x18: 0 until tgtSelectManual opens the target menu, then its mode;
                                             * 7, 10, 12, 14 and 16 can use targets[1] */
    int phase;                              /* +0x1C */
    int selection;                          /* +0x20: battleSelectStep's choice (0 none, TGT_SELECTION_BACK) */
    int selectionKind;                      /* +0x24: set by battleSelectStep; battleSelectionGet takes 1 and 2 */
    int active;                             /* +0x28 */
    int selectionIndex;                     /* +0x2C: the record the menu selection picked (battleSelectionGet) */
    int status;                             /* +0x30: TGT_STATUS_* (tu035) */
    int victoryStarted;                     /* +0x34: set once battleEndCheck has started the victory */
    struct Ov01Object *object;              /* +0x38: cleared at the end of a turn */
    int first;                              /* +0x3C */
    int second;                             /* +0x40 */
    int third;                              /* +0x44: object/first/second/third are what unitCmdBatchRun passes on */
    int transition;                         /* +0x48: 1 when the motion mode is below 16 (a party unit left), else 2 */
    int sideToggle;                         /* +0x4C: flipped by L1/L2 in the cursor stage */
    int moveCmd;                            /* +0x50: set by setMoveCmd */
    int mode;                               /* +0x54 */
    int slotIndex;                          /* +0x58: the index battleEntryActivate and unitRecreate use */
    float scale;                            /* +0x5C: the angle unitRecreate restores */
    unsigned char *targetFlags;             /* +0x60: bit 0 of its first byte selects targets[1] */
};

extern BattleState tgtSelectState;

struct BattleState *battleStateGet(void);
struct CmdListBody;
int battleSelectionGet(struct Ov01Object *unit, struct CmdListBody *command, struct Ov01Object *target);

short tableKeyEntryGet(struct Ov01Object *owner, int index, void *target);
void gattaiCodeSet(int code);
void thinkNoSet(int value);

#endif /* INCLUDE_OV01_TU036_H */
