/*
 * TiWatSim: the water height simulation (file ../TiWatSim.euc.c in the assert strings).
 * A table holds a count at +0x08 and items from +0x10, 0x110 bytes each. An item keeps a
 * cell grid: cols at +0x08, rows at +0x0C, cell spacing at +0x50, drop amount at +0x58,
 * height buffer A at +0x68, buffer B at +0x6C (read for the normals), a mask at block+0x78,
 * normals at block+0x7C and the block base at +0x84. Names are proposals (see the proposal file).
 */
#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimPadDrop);

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimComputeNormals);

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimStepRain);

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimDisturbAt);

#include "shared.h"

/* Same shape as TiWaterSub in main/TiWater.c (magic "WAT " at +0x00, count at +0x08). */
typedef struct TiWatSimTable {
    u8 unmodeled_00[0x08];
    u32 count;
    u8 unmodeled_0C[0x04];
} TiWatSimTable;

typedef struct TiWatSimItem {
    u8 unmodeled_00[0x110];
} TiWatSimItem;

/* The second argument is a pointer to a position vector (the assert names it pPos); it is typed int here. */
void tiWatSimDisturbAt(TiWatSimItem *item, int value);

/* Disturbs every item of the table at the position pointed to by value. */
void tiWatSimDisturbAll(TiWatSimTable *table, int value)
{
    TiWatSimItem *item;
    u32 i;

    item = (TiWatSimItem *) (table + 1);
    for (i = 0; i < table->count; i++) {
        tiWatSimDisturbAt(item, value);
        item++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimRandomizeItem);

/* Gives each masked cell of the item a random height in a band scaled by range and clears buffer B. */
void tiWatSimRandomizeItem(TiWatSimItem *item, int range);

/* Randomizes every item of the table with the same range. */
void tiWatSimRandomizeAll(TiWatSimTable *table, int range)
{
    TiWatSimItem *item;
    u32 i;

    item = (TiWatSimItem *) (table + 1);
    for (i = 0; i < table->count; i++) {
        tiWatSimRandomizeItem(item, range);
        item++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", tiWatSimStoreSourcePos);

INCLUDE_ASM("asm/main/nonmatchings/TiWatSim", func_00240320);

/* Bump allocator: returns the cursor and advances it by size, rounded up to 16 bytes. */
u32 tiWatSimAllocAligned16(u32 *cursor, u32 size)
{
    u32 old;

    old = *cursor;
    *cursor = (old + size + 0xF) & ~0xF;
    return old;
}

/* Bump allocator without alignment: returns the cursor and advances it by size. */
u32 tiWatSimAlloc(u32 *cursor, u32 size)
{
    u32 old;

    old = *cursor;
    *cursor = old + size;
    return old;
}
