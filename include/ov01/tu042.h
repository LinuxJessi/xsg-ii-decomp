#ifndef INCLUDE_OV01_TU042_H
#define INCLUDE_OV01_TU042_H

/* Functions of src/ov01/tu042.c that other files call. */

/* Effect request block sent to the main-side effect queue (sefStartEffectDefault /
 * sefStartEffectWithValue); the senders clear it first. */
struct EffectRequest {
    struct XenoActor *head;            /* +0x00: the actor of the owner unit (Ov01Object.unit) */
    unsigned char unmodeled_04[0x04];
    struct XenoActor *target;          /* +0x08: the same actor again (tu033 senders) */
    unsigned char unmodeled_0c[0x1C - 0x0C];
    int key;                           /* +0x1C: (high << 12) | low, see createEffect */
    unsigned char unmodeled_20[0x2C - 0x20];
    short mode;                        /* +0x2C: 2 or 3 (func_00A95540) */
    unsigned char unmodeled_2e[0x38 - 0x2E];
};

/* Unaligned signed word: the record's word sits at a 2-byte offset. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* One 4-byte entry of the unit experience table unitExpTblGet returns. */
typedef struct UnitExpEntry UnitExpEntry;

struct UnitExpEntry {
    UnalignedS32 exp;
};

/* One pair of halfwords of a row slotPairRowGet returns. */
typedef struct ShortPair ShortPair;

struct ShortPair {
    unsigned short first;
    unsigned short second;
};

/*
 * The 0x20-byte growth record growthRecordGet returns for a party slot key: the start values
 * battleSlotsInit loads into the slot record's growth values, and the targets unitGrowthStep
 * steps them toward.
 */
typedef struct GrowthRecord GrowthRecord;

struct GrowthRecord {
    unsigned short start[8];                /* +0x00: the growth values a slot starts with */
    unsigned short target[8];               /* +0x10: the values the growth values of the slot record move toward */
};

/* Partial view of a 16-byte table key record tableKeyRecordGet returns: points, rates, chances and the two (category, id) drop pairs. */
struct TableKeyRecord {
    int points;                    /* +0x00 */
    short stepRate;                /* +0x04 */
    short flat;                    /* +0x06 */
    unsigned char secondChance;    /* +0x08 */
    unsigned char firstChance;     /* +0x09 */
    unsigned char secondCategory;  /* +0x0A: the data box category of the second pair, zero when it drops nothing */
    unsigned char firstCategory;   /* +0x0B: the data box category of the first pair */
    unsigned char secondId;        /* +0x0C: the data box id of the second pair */
    unsigned char firstId;         /* +0x0D: the data box id of the first pair */
};

/* The script attack table the natives atkTbl and atkTblGet (tu046) take from the image's globals: a row count,
 * then rows of two words. */
typedef struct AtkTblRow {
    int key;   /* +0x00: atkTblGet field 1 */
    int value; /* +0x04: atkTblGet field 2 */
} AtkTblRow;

typedef struct AtkTbl {
    int count;          /* +0x00: atkTblGet field 0 (any other field) */
    AtkTblRow rows[1];  /* +0x04: extent given by count */
} AtkTbl;

struct Ov01Object;

/* The 16-byte record func_00A9BCC8 takes: the unit and three words it passes on, with the selector, as a script
 * message (sbThreadEnqueue). */
typedef struct UnitScriptEvent {
    struct Ov01Object *unit;
    int words[3];
} UnitScriptEvent;

int atkTblFieldGet(int field, AtkTbl *table, int index);
int func_00A9BF08(struct Ov01Object *unit, AtkTbl *table, int value);
void thinkSendEnd(struct Ov01Object *unit);
void thinkDispatch(struct Ov01Object *unit, int command, int arg, struct Ov01Object *target);

/* The records of the bat.dat tables (the getters below, numbered from 1). */
/* The entry battleLookupGet returns. */
typedef struct BattleLookupEntry BattleLookupEntry;

struct BattleLookupEntry {
    unsigned char unmodeled_00[6];
    unsigned short flags;                   /* +0x06 */
    short tecIndex;                         /* +0x08: the index tecRecordGet takes */
    unsigned char firstCode;                /* +0x0A */
    unsigned char secondCode;               /* +0x0B */
};

typedef struct TecRecord TecRecord;

/* The 0x20-byte technique record tecRecordGet returns for an action index. */
struct TecRecord {
    /* +0x00: target flags (1, 2, 3 side, 4 self, 8 all, 0x10 flagged units); bit 0x20 is tested with
     * calcStatGet(unit, 4, 0x2000) */
    unsigned char flags;
    signed char code;                       /* +0x01: func_00252CD8's code; must be 0 for calcStatusHitRoll */
    short scale;                            /* +0x02: percent scale battleEffectApplies passes to func_00252CD8 */
    signed char rollThreshold;              /* +0x04: a first roll below this goes to the second roll */
    signed char secondRollThreshold;        /* +0x05: a second roll below this returns 0 */
    signed char type;                       /* +0x06: compared with 2 by calcHitRecordFill */
    union {
        signed char kind;                   /* kind byte, mapped by calcHitMode and calcKindHitMode */
        unsigned char effectFlags;          /* the same byte as flags 0x1 to 0x40, read by calcHitApply */
        signed char slot;                   /* the slot func_00A92D98 (tu033) accepts below 8 */
    } kindBits;                             /* +0x07 */
    unsigned short hitFlags;                /* +0x08: bits 0x4, 0x8, 0x10 and 0x20 are tested; 0x100 sets object
                                             * flag 0x2000 (unitCmdSetPhase) */
    signed char epCost;                     /* +0x0A: the EP cost calcEpCost scales */
    signed char levelDelta;                 /* +0x0B: unitRecordStateGet's result */
    /* +0x0C: tested against 0x100, 0x200, 0x500 and 0x600 by calcHitStateStep; the low five bits are
     * calcComboUpdate's mask */
    unsigned short actionBits;
    unsigned char power;                    /* +0x0E */
    signed char modeGuard[3];               /* +0x0F..+0x11: bit 0 of modeGuard[mode] blocks the hit */
    /* +0x12..+0x15: one gate byte per row slot (calcHitRowGate), the unit-state codes battleEffectApplies
     * tests; bit 0x10 of the first clears object flag 0x2000 (unitCmdSetPhase) */
    signed char rowGate[4];
    signed char inflictRate;                /* +0x16: chance the value of calcStatResist is taken from */
    signed char row;                        /* +0x17 */
    unsigned short bits;                    /* +0x18: one bit per status of the row */
    short extent;                           /* +0x1A: the extent func_00A93288 (tu033) passes to unitEffectKeyFree */
    /* +0x1C: zero lets calcHitRowGate walk its row gates; unitSlotsClear tests 5, unitAttachRequest 1 */
    signed char gateFlag;
    signed char phase;                      /* +0x1D: the phase unitCmdSetPhase (tu012) sets */
    short timer;                            /* +0x1E: nonzero selects unitSetTimedPhase */
};

/* The record statusRecordGet returns for a status or effect index. */
typedef struct StatusRecord StatusRecord;

struct StatusRecord {
    unsigned char unmodeled_00[0x6];        /* +0x00..+0x05 */
    /* +0x06: bit 0x2000 marks an effect that lives in the slot table; 0x8000 and 0x4000 are tested by
     * battleSlotExport */
    unsigned short slotFlags;
    unsigned short flags;                   /* +0x08: tested with the caller's mask */
    unsigned short code;                    /* +0x0a: low 7 bits are the result */
};

/* The equipment item record itemRecordGet returns: its code halfword is at +0xA. */
typedef struct ItemRecord ItemRecord;

struct ItemRecord {
    unsigned char unmodeled_00[0xA];
    short code;                             /* +0xA */
};

typedef struct HitAction HitAction;

/* Action row hitActionGet returns: the entry index and the amount. */
struct HitAction {
    unsigned char unmodeled_00[0x4];        /* +0x00..+0x03 */
    unsigned short flags;                   /* +0x04: tested by dataSlotKeyTest (tu022) */
    unsigned char unmodeled_06[0x2];        /* +0x06..+0x07 */
    short entry;                            /* +0x08: index tecRecordGet takes */
    unsigned char unmodeled_0a[0x2];        /* +0x0A..+0x0B */
    short amount;                           /* +0x0C: selects the +0x40 action; nonzero picks row 2 */
};

/* Link record pairLinkGet returns: the required level and the two slot indices it pairs. */
typedef struct PairLink PairLink;

struct PairLink {
    unsigned char unmodeled_00[0x8];
    short tecIndex;                         /* +0x8: the technique a pair selection uses (battleSelectionGet) */
    short requiredLevel;                    /* +0xA */
    short slotFirst;                        /* +0xC */
    short slotSecond;                       /* +0xE */
};

typedef struct CalcLevelRecord CalcLevelRecord;

/* The record calcLevelRecordGet returns; +0xA is its required level, +0xC the entry mask calcEntryMaskTest tests. */
struct CalcLevelRecord {
    unsigned char unmodeled_000[0x8];
    short tecIndex;                         /* +0x8: the technique a level selection uses (battleSelectionGet) */
    short requiredLevel;
    short entryMask;
};

typedef struct TgtIdRecord TgtIdRecord;

/* The record tgtIdRecordGet returns for an id: +0x8 is the action index tecRecordGet takes, +0xE the minimum level. */
struct TgtIdRecord {
    unsigned char unmodeled_00[0x8];
    short actionIndex;                      /* +0x08: the index tecRecordGet takes */
    unsigned char unmodeled_0a[0xe - 0xa];
    short minLevel;                         /* +0x0E: obj->stats->level below this gives 0 */
};

/* The unit parameters scripts read and write by number (natives unitParaGet and unitParaSet, tu046). 17..36 are
 * the bytes from StatBlock.liveRows on: liveRows (17..20), slotValue (21..28) and slotRoll (29..36). */
enum {
    UNIT_PARA_MAX_HP,
    UNIT_PARA_MAX_EP,
    UNIT_PARA_PHYS_ATTACK,
    UNIT_PARA_PHYS_DEFENCE,
    UNIT_PARA_MAG_ATTACK,
    UNIT_PARA_MAG_DEFENCE,
    UNIT_PARA_HIT_RATE,
    UNIT_PARA_EVADE_RATE,
    UNIT_PARA_VALUE_BIAS,
    UNIT_PARA_HP,
    UNIT_PARA_EP,
    UNIT_PARA_KEY,
    UNIT_PARA_STATE70,                  /* read only */
    UNIT_PARA_LEVEL,
    UNIT_PARA_EXP_LEVEL,
    UNIT_PARA_FLAGS,
    UNIT_PARA_POS_INDEX,
    UNIT_PARA_LIVE_ROW,
    UNIT_PARA_SLOT_VALUE = UNIT_PARA_LIVE_ROW + 4,
    UNIT_PARA_SLOT_ROLL = UNIT_PARA_SLOT_VALUE + 8,
    UNIT_PARA_CURRENT = UNIT_PARA_SLOT_ROLL + 8
};

int unitParaFieldGet(struct Ov01Object *unit, int field);
void unitParaFieldSet(struct Ov01Object *unit, int field, int value);

/* Table getters: each returns record index of its bat.dat table, 0 for an index outside the table. */
TecRecord *tecRecordGet(int index);
void *tableKeyRowGet(int key);
short *effectIdListGet(int index);
ShortPair *slotPairRowGet(int index);
BattleLookupEntry *battleLookupGet(int index);
ItemRecord *itemRecordGet(int index);
StatusRecord *statusRecordGet(int index);
HitAction *hitActionGet(int index);
void *func_00A9B580(int index);
void *func_00A9B5B8(int index);
PairLink *pairLinkGet(int index);
CalcLevelRecord *calcLevelRecordGet(int index);
TgtIdRecord *tgtIdRecordGet(int index);
UnitExpEntry *unitExpTblGet(void);
GrowthRecord *growthRecordGet(int index);
short *stringOffsetsGet(int index);
char *tableStringGet(int index, int mode);

/* Message, party data and table row prototypes. */
int msgIsIdle(void);
void partyDataSetSlot(int id, int slot, int value);
struct TableKeyRecord *tableKeyRecordGet(int key);

#endif /* INCLUDE_OV01_TU042_H */
