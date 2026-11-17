#ifndef INCLUDE_OV01_TU042_H
#define INCLUDE_OV01_TU042_H

/* Functions of src/ov01/tu042.c that other files call. */

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
