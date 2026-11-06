#include "common.h"
#include "main/TiMRPackage.h"

/*
 * Counted table of entries: the entry count sits at +8 and a table of byte
 * offsets, relative to the table itself, starts at +0xC.
 */
typedef struct OffsetTable OffsetTable;

struct OffsetTable {
    unsigned char unmodeled_00[8];
    unsigned int count;      /* +8 */
    int entryOffsets[1];     /* +0xC: byte offset of entry i from the table;
                              * the count at +8 bounds the indexes used. */
};

/*
 * Partial views of the two owners. Both are the TiMRPackage of TiMRPackage.c:
 * owner A binds state A (at +0x10) to the table at +0xC4, owner B binds state B
 * (at +0x60) to the table at +0xCC. Each keeps its own table pointer and the
 * state block whose address is passed to tiWordSetBits (flag 0x40).
 */
typedef struct OffsetTableOwnerA OffsetTableOwnerA;

struct OffsetTableOwnerA {
    unsigned char unmodeled_00[0x10];
    unsigned char stateA[0xC4 - 0x10]; /* +0x10, passed by address */
    OffsetTable *entryTable;           /* +0xC4 */
};

typedef struct OffsetTableOwnerB OffsetTableOwnerB;

struct OffsetTableOwnerB {
    unsigned char unmodeled_00[0x60];
    unsigned char stateB[0xCC - 0x60]; /* +0x60, passed by address */
    OffsetTable *entryTable;           /* +0xCC */
};

void tiMRPackageSetEntryA(void *pMRPackage, unsigned char *entry);
void tiMRPackageSetEntryB(void *pMRPackage, unsigned char *entry);

/* Binds entry `index` of table A to state A and sets flag 0x40 of state A. */
void tiMRPackageSelectEntryA(OffsetTableOwnerA *owner, unsigned int index) {
    OffsetTable *table;
    int entryOffset;

    table = owner->entryTable;
    if (table == 0) {
        return;
    }
    if (index < table->count) {
        entryOffset = index[table->entryOffsets];
        tiMRPackageSetEntryA(owner, (unsigned char *) table + entryOffset);
        tiWordSetBits(owner->stateA, 0x40);
    }
}

/* Stores table A (the argument), then binds entry `index` of it to state A as tiMRPackageSelectEntryA. */
INCLUDE_ASM("asm/main/nonmatchings/tu253", tiMRPackageSetTableAndEntryA);

/* Binds entry `index` of table B to state B and sets flag 0x40 of state B. */
void tiMRPackageSelectEntryB(OffsetTableOwnerB *owner, unsigned int index) {
    OffsetTable *table;
    int entryOffset;

    table = owner->entryTable;
    if (table == 0) {
        return;
    }
    if (index < table->count) {
        entryOffset = index[table->entryOffsets];
        tiMRPackageSetEntryB(owner, (unsigned char *) table + entryOffset);
        tiWordSetBits(owner->stateB, 0x40);
    }
}
