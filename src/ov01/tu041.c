/*
 * OV01 original TU 41: 0x00a9b120..0x00a9b280 (4 functions)
 */
#include "common.h"
#include "ov01/tu041.h"

/* One 16-byte record of the effect position table: x, y, z in 1/100 units (createEffectPos), then a word that is
 * EFFECT_POS_END in the record that ends the table. */
typedef struct EffectPosRecord {
    int position[3]; /* +0x00 */
    int end;         /* +0x0C */
} EffectPosRecord;

#define EFFECT_POS_END -1

/* Base of the effect position table, set by effectPosTableSet. */
extern EffectPosRecord *effectPosTable;
/* Number of valid position records (the index of the end record). */
extern int effectPosCount;

/* Sets the effect position table: the base goes to effectPosTable, the number of records before the end record to
 * effectPosCount. */
void effectPosTableSet(EffectPosRecord *table)
{
    effectPosTable = table;
    effectPosCount = 0;
    while (table[effectPosCount].end != EFFECT_POS_END) {
        effectPosCount++;
    }
}

/*
 * Returns position record `index` (x, y, z in 1/100 units, read by createEffectPos), or 0 when index is past the
 * last record.
 */
int *effectPosGet(int index) {
    if (!(index < effectPosCount)) {
        return 0;
    }
    return effectPosTable[index].position;
}

/* Number of valid effect position records. */
int effectPosGetCount(void) {
    return effectPosCount;
}

/*
 * Resolves an index to a value over four ranges: below 0xB, 0xB..0xF,
 * 0x10..0x8C (tables ov01PartyList and D_00AB5CF0) and 0x8D and up
 * (gattaiCodeGet). The meaning of the index is not known yet.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu041", func_00A9B1B8);
