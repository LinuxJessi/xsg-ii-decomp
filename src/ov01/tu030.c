/*
 * OV01 original TU 30: 0x00a90660..0x00a912d0 (18 functions)
 */
#include "common.h"
#include "ov01/tu043.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90660);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A907B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", tgtSideOf);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90858);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90988);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90A18);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90A88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", tgtCollectUnits);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90CD8);

/* The record func_00A9B548 returns for an id; +0x8 is the index func_00A9B3B8 takes. */
struct CalcIdRecord {
    unsigned char unmodeled_000[0x8];
    short index;
};

int tgtCollectUnits(int target, unsigned char flags, int);
unsigned char *func_00A9B3B8(short index);
struct CalcIdRecord *func_00A9B548(int id);

/* Whether target is a valid target of the record id: the record's flags byte selects the units (tgtCollectUnits). */
int tgtRecordCanTarget(int target, int id)
{
    struct CalcIdRecord *lookup;
    unsigned char flags;

    lookup = func_00A9B548(id);
    flags = *func_00A9B3B8(lookup->index);
    if (dataBoxChkCategory1(id) <= 0) {
        return 0;
    }
    return tgtCollectUnits(target, flags, 0) != 0;
}

extern int D_00AB5A14[];

int calcEntryMaskTest(int index, int mask)
{
    if (index == 0 || index >= 11) {
        return 0;
    }
    return (D_00AB5A14[index] & mask) != 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90E38);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90F70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A90FB0);

/* The object actStockGet returns; +0x14 holds the current entity (the acting unit). */
struct CalcActState {
    unsigned char unmodeled_000[0x14];
    struct CalcEntity *current;
};

/* An entity; +0x144 points at its body. */
struct CalcEntity {
    unsigned char unmodeled_000[0x144];
    struct CalcEntityBody *body;
};

/* +0x71 is the level compared against a record's required level. */
struct CalcEntityBody {
    unsigned char unmodeled_000[0x71];
    signed char level;
};

/* The record func_00A9B5F0 returns; +0xA is its required level. */
struct CalcLevelRecord {
    unsigned char unmodeled_000[0xA];
    short required_level;
};

struct CalcActState *actStockGet(void);
struct CalcLevelRecord *func_00A9B5F0(int index);

/* Whether both the entity and the acting unit reach the level record record_index requires. */
int calcLevelsMeetRecord(struct CalcEntity *entity, int record_index)
{
    struct CalcEntity *current;
    struct CalcLevelRecord *record;

    current = actStockGet()->current;
    if (current == 0) {
        return 0;
    }
    record = func_00A9B5F0(record_index);
    if (entity->body->level < record->required_level || current->body->level < record->required_level) {
        return 0;
    }
    return 1;
}

/* The pair of codes func_00A9B490 returns; +0xA and +0xB are the two codes. */
struct CalcCodePair {
    unsigned char unmodeled_000[0xA];
    unsigned char first_code;
    unsigned char second_code;
};

int slotCodeTest(struct CalcEntity *entity, int code);
struct CalcCodePair *func_00A9B490(int index);

/*
 * Collects into found the pair records 60..89 that hold code and whose partner code the acting unit satisfies,
 * and returns how many.
 */
int calcPairedRecords(int unused_arg0, int code, int *found)
{
    struct CalcEntity *current;
    unsigned char first_code;
    unsigned char second_code;
    int count;
    int index;

    count = 0;
    current = actStockGet()->current;
    if (current == 0) {
        return 0;
    }
    for (index = 60; index < 90; index++) {
        first_code = func_00A9B490(index)->first_code;
        second_code = func_00A9B490(index)->second_code;
        if ((code == first_code && slotCodeTest(current, second_code) != 0) ||
            (code == second_code && slotCodeTest(current, first_code) != 0)) {
            found[count] = index;
            count++;
        }
    }
    return count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu030", func_00A911F0);

int func_00A8F560(int subject, int index);

int slotFlagCollect(int subject, int *found)
{
    int count;
    int index;

    count = 0;
    for (index = 1; index < 11; index++) {
        if (func_00A8F560(subject, index) != 0) {
            found[count] = index;
            count++;
        }
    }
    return count;
}
