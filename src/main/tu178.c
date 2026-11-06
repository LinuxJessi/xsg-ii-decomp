#include "common.h"
#include "main/sv_get_image_item_159.h"

/* Loads an effect package file ("esp/cfdef.esp" when cfdef is set, else
 * "esp/default.esp") into the 0x28800-byte block of the first record of
 * srsRecordTable. A reload skips the tag check and always reads the file. */
INCLUDE_ASM("asm/main/nonmatchings/tu178", srsLoadEspFile);

void srsLoadEspFile(int reload, int cfdef);

/* Loads the default effect package. */
void srsLoadDefaultEsp(void)
{
    srsLoadEspFile(0, 0);
}

int srsGetEffectFileKind(int, int);

/* Decodes an effect key: index = bits 12..21, kind = bits 28..29. Returns the
 * index when srsGetEffectFileKind accepts the pair, else 0. */
int srsCheckEffectKey(int effectKey)
{
    int index;
    int kind;

    index = (effectKey >> 0xC) & 0x3FF;
    kind = (effectKey >> 0x1C) & 3;
    if (srsGetEffectFileKind(index, kind) != 1) {
        return 0;
    }
    return index;
}

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsReleaseSlotsByIndex);

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsReleaseEffectRecords);

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsReleaseAllSlots);

void srsReleaseEffectRecords(void);
void srsReleaseAllSlots(void);

void srsReleaseRecords(void)
{
    srsReleaseAllSlots();
    srsReleaseEffectRecords();
}

void *memset(void *s, int c, unsigned int n);
void smFree(void *block);
extern void *srsRecordTable;

void srsReleaseEspFile(void)
{
    srsReleaseRecords();
    if (srsRecordTable != 0) {
        svDeleteImageMapper(0);
        smFree(srsRecordTable);
        memset(&srsRecordTable, 0, 0x1C);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsMapPendingResources);

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsLoadPendingEffects);

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsLoadEffectBlock);
