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

/* One pair of halfwords of a row func_00A9B450 returns. */
typedef struct ShortPair ShortPair;

struct ShortPair {
    unsigned short first;
    unsigned short second;
};

/*
 * The 0x20-byte growth record func_00A9B6E8 returns for a party slot key: the start values
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

int atkTblFieldGet(int field, AtkTbl *table, int index);
int func_00A9BF08(struct Ov01Object *unit, AtkTbl *table, int value);
void thinkSendEnd(struct Ov01Object *unit);

/* Table and record prototypes. */
UnitExpEntry *unitExpTblGet(void);
GrowthRecord *func_00A9B6E8(int index);
ShortPair *func_00A9B450(int index);

/* Message, party data and table row prototypes. */
int msgIsIdle(void);
void partyDataSetSlot(int id, int slot, int value);
void * tableKeyRowGet(int key);
struct TableKeyRecord *tableKeyRecordGet(int key);

#endif /* INCLUDE_OV01_TU042_H */
