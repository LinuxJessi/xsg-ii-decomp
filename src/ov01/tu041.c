/*
 * OV01 original TU 41: 0x00a9b120..0x00a9b280 (4 functions)
 */
#include "common.h"
#include "ov01/tu041.h"

/*
 * Sets the effect position table: the base goes to effectPosTable and the count
 * of 16-byte records goes to effectPosCount. A record is terminated by a word
 * of -1 at +0xC (the last word of the 16-byte record).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu041", effectPosTableSet);

/* Base of the effect position table: 16-byte records, set by effectPosTableSet. */
extern int effectPosTable;
/* Number of valid position records (the index of the -1 terminator). */
extern int effectPosCount;

/*
 * Returns position record `index` (words 0, 1, 2 are x, y, z in 1/100 units,
 * read by createEffectPos), or 0 when index is past the last record.
 */
int *effectPosGet(int index) {
    if (!(index < effectPosCount)) {
        return 0;
    }
    return effectPosTable + (index * 0x10);
}

/* Number of valid effect position records. */
int effectPosGetCount(void) {
    return effectPosCount;
}

/*
 * Resolves an index to a value over four ranges: below 0xB, 0xB..0xF,
 * 0x10..0x8C (tables ov01PartyList and D_00AB5CF0) and 0x8D and up
 * (func_00A99E18). The meaning of the index is not known yet.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu041", func_00A9B1B8);
