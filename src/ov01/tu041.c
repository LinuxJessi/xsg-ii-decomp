/*
 * OV01 original TU 41: 0x00a9b120..0x00a9b280 (4 functions)
 */
#include "common.h"
#include "main/tu110.h"
#include "ov01/tu023.h"
#include "ov01/tu036.h"
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

/* Unit keys: 1..10 party members, 11..15 the large units (tu008 gives them DATA_UNIT_HEAP_LARGE), 16..140 the
 * other data ids; keys from UNIT_KEY_COUNT up are gattai codes. */
#define UNIT_KEY_LARGE_FIRST 11
#define UNIT_KEY_OTHER_FIRST 16
#define UNIT_KEY_COUNT 141

/* Unit effect code that switches a party member to the file of unitEffectFileIds. */
#define UNIT_EFFECT_FILE_SWAP 0x3F

/* Model file ids by unit key: party members with UNIT_EFFECT_FILE_SWAP (key - 1, 0 for none), the large units
 * (key - UNIT_KEY_LARGE_FIRST) and the other data ids (key - UNIT_KEY_OTHER_FIRST). */
extern int unitEffectFileIds[];
extern int largeUnitFileIds[];
extern int otherUnitFileIds[];

/* Model file id of unit `key` (tu008 builds the file name from it): a party member's cloth id from ptStatusBlock
 * (or its unitEffectFileIds entry under UNIT_EFFECT_FILE_SWAP), a table entry for the others, the gattai code past
 * the data ids. */
int unitFileIdGet(int key)
{
    int fileId;
    int effectFileId;

    if (key < UNIT_KEY_LARGE_FIRST) {
        fileId = ptGetStatusBlock()->clothIds[key - 1];
        if (battleUnitHasEffectCode(key, UNIT_EFFECT_FILE_SWAP)) {
            effectFileId = unitEffectFileIds[key - 1];
            if (effectFileId != 0) {
                fileId = effectFileId;
            }
        }
    } else if (key < UNIT_KEY_OTHER_FIRST) {
        fileId = largeUnitFileIds[key - UNIT_KEY_LARGE_FIRST];
    } else if (key >= UNIT_KEY_COUNT) {
        fileId = gattaiCodeGet();
    } else {
        fileId = otherUnitFileIds[key - UNIT_KEY_OTHER_FIRST];
    }
    return fileId;
}
