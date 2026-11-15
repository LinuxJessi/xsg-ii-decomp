#include "common.h"
#include "main/sv_get_image_item_159.h"
#include "main/tu173.h"

/* One 0x1C-byte record of srsRecordTable: record 0 holds the effect package
 * (image mapper 0), records 1..8 the pending-resource slots (mappers 1..8),
 * records 9..56 the effect records (mappers 9..56). */
typedef struct SrsRecord {
    void *buffer;                      /* +0x00 */
    int key;                           /* +0x04 */
    unsigned char unmodeled_08[0x18 - 0x08];
    short effectNo;                    /* +0x18 */
    signed char freeKind;              /* +0x1A: 0 smFree, 1 GameResBlockFreeByBuffer */
    signed char mapPending;            /* +0x1B: svAddImageMapper still owed */
} SrsRecord;

/* srsRecordTable: 58 records (srsResetKeyTable in srs.c clears 0x658 bytes);
 * this file never touches the last one. */
typedef struct SrsRecordTable {
    SrsRecord esp;
    SrsRecord slots[8];
    SrsRecord effects[49];
} SrsRecordTable;

extern SrsRecordTable srsRecordTable;

#define SRS_ESP_BUFFER_SIZE 0x28800  /* buffer of the effect package (record 0) */
#define SRS_SLOT_BUFFER_SIZE 0x1C000 /* buffer of a pending-resource slot */

/* Defined as void smAlloc(int) in sm_initilize.c; the allocation comes back in $v0. */
void *smAlloc(int size);
int fileLoad(void *buffer, const char *name, int mode);
extern const char srsEspCfdefPath[];
extern const char srsEspDefaultPath[];
void srsMapPendingResources(void);

/* Loads an effect package file ("esp/cfdef.esp" when cfdef is set, else
 * "esp/default.esp") into the buffer of record 0 of srsRecordTable, allocating
 * it on first use. When the buffer already holds the package with the same key
 * and reload is 0, only its image mapper is registered again; otherwise the
 * file is read again. */
void srsLoadEspFile(int reload, int cfdef)
{
    const char *path;
    int key;

    if (cfdef != 0) {
        path = srsEspCfdefPath;
    } else {
        path = srsEspDefaultPath;
    }
    /* Effect key of the package: index 999, sub number 500, kind 1 for cfdef. */
    key = cfdef ? 0x103E71F4 : 0x3E71F4;
    if (srsRecordTable.esp.buffer == 0) {
        srsRecordTable.esp.buffer = smAlloc(SRS_ESP_BUFFER_SIZE);
        if (srsRecordTable.esp.buffer == 0) {
            return;
        }
        srsRecordTable.esp.mapPending = 1;
        srsRecordTable.esp.key = key;
        if (fileLoad(srsRecordTable.esp.buffer, path, 0) >= 0) {
            if (cfdef != 0) {
                xglCdWaitUntilIdle();
            }
            return;
        }
        srsRecordTable.esp.mapPending = 0;
        srsRecordTable.esp.key = 0;
        if (srsRecordTable.esp.buffer == 0) {
            return;
        }
    }
    svDeleteImageMapper(0);
    if (reload == 0 && srsRecordTable.esp.key == key) {
        srsRecordTable.esp.mapPending = 1;
        srsMapPendingResources();
        return;
    }
    srsRecordTable.esp.mapPending = 1;
    srsRecordTable.esp.key = key;
    if (fileLoad(srsRecordTable.esp.buffer, path, 0) >= 0) {
        if (cfdef != 0) {
            xglCdWaitUntilIdle();
        }
    } else {
        srsRecordTable.esp.mapPending = 0;
        srsRecordTable.esp.key = 0;
    }
}

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

void *memset(void *s, int c, unsigned int n);
void smFree(void *block);

/* Clears every pending-resource slot (records 1..8) whose key index equals
 * index, releasing its image mapper and buffer first when it has one. */
void srsReleaseSlotsByIndex(int index)
{
    int keyIndex;
    int i;

    for (i = 0; i < 8; i++) {
        keyIndex = (srsRecordTable.slots[i].key >> 12) & 0x3FF;
        if (keyIndex != 0 && keyIndex == index) {
            if (srsRecordTable.slots[i].buffer != 0) {
                svDeleteImageMapper(i + 1);
                smFree(srsRecordTable.slots[i].buffer);
            }
            memset(&srsRecordTable.slots[i], 0, sizeof(SrsRecord));
        }
    }
}

int GameResBlockFreeByBuffer(void *buffer);

/* Releases records 9..56 (image mapper i + 9): the buffer goes back to smFree
 * or GameResBlockFreeByBuffer by freeKind, then the record is cleared. */
void srsReleaseEffectRecords(void)
{
    int i;

    for (i = 0; i < 48; i++) {
        if (srsRecordTable.effects[i].buffer != 0) {
            svDeleteImageMapper(i + 9);
            if (srsRecordTable.effects[i].freeKind == 0) {
                smFree(srsRecordTable.effects[i].buffer);
            } else if (srsRecordTable.effects[i].freeKind == 1) {
                GameResBlockFreeByBuffer(srsRecordTable.effects[i].buffer);
            }
            memset(&srsRecordTable.effects[i], 0, sizeof(SrsRecord));
        }
    }
}

/* Releases every pending-resource slot (records 1..8) that holds a buffer. */
void srsReleaseAllSlots(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (srsRecordTable.slots[i].buffer != 0) {
            svDeleteImageMapper(i + 1);
            smFree(srsRecordTable.slots[i].buffer);
            memset(&srsRecordTable.slots[i], 0, sizeof(SrsRecord));
        }
    }
}

void srsReleaseRecords(void)
{
    srsReleaseAllSlots();
    srsReleaseEffectRecords();
}

void srsReleaseEspFile(void)
{
    srsReleaseRecords();
    if (srsRecordTable.esp.buffer != 0) {
        svDeleteImageMapper(0);
        smFree(srsRecordTable.esp.buffer);
        memset(&srsRecordTable.esp, 0, sizeof(SrsRecord));
    }
}

/* Registers the image mapper of every loaded record still flagged
 * mapPending: the effect package (mapper 0, tag 3000), the pending-resource
 * slots (mappers 1..8, tag 2000, entry count from key bits 28..29) and the
 * effect records (mappers 9..56, tag 3600). */
void srsMapPendingResources(void)
{
    void *chunk[8]; /* svAnalyzeChunk reads only chunk[0]; the 0x20 bytes are the original frame */
    int i;

    if (srsRecordTable.esp.buffer != 0 && srsRecordTable.esp.mapPending != 0) {
        chunk[0] = srsRecordTable.esp.buffer;
        svAddImageMapper(0, 0, chunk, 3000);
        srsRecordTable.esp.mapPending = 0;
    }
    for (i = 0; i < 8; i++) {
        if (srsRecordTable.slots[i].buffer != 0 && srsRecordTable.slots[i].mapPending != 0) {
            chunk[0] = srsRecordTable.slots[i].buffer;
            svAddImageMapper(i + 1, (srsRecordTable.slots[i].key >> 28) & 3, chunk, 2000);
            srsRecordTable.slots[i].mapPending = 0;
        }
    }
    for (i = 0; i < 48; i++) {
        if (srsRecordTable.effects[i].buffer != 0 && srsRecordTable.effects[i].mapPending != 0) {
            chunk[0] = srsRecordTable.effects[i].buffer;
            svAddImageMapper(i + 9, 0, chunk, 3600);
            srsRecordTable.effects[i].mapPending = 0;
        }
    }
}

/* sefSlotTable (tu165): the effect number of each of the eight wait slots and
 * the flag that asks srsLoadPendingEffects to load its package. */
typedef struct SefSlotTable {
    short effectNo[8];                 /* +0x00 */
    unsigned char unmodeled_10[0x20 - 0x10];
    short loadPending[8];              /* +0x20 */
} SefSlotTable;

char *srsGetEspName(int effectNo);

/* Loads the .esp package of every wait slot flagged loadPending into the
 * buffer of pending-resource slot i, flags it mapPending, and sets
 * its key when the file fits. */
void srsLoadPendingEffects(SefSlotTable *slotTable)
{
    char *name;
    int size;
    int i;

    for (i = 0; i < 8; i++) {
        if (slotTable->effectNo[i] > 0 && slotTable->loadPending[i] != 0) {
            slotTable->loadPending[i] = 0;
            name = srsGetEspName(slotTable->effectNo[i] << 12);
            if (srsRecordTable.slots[i].buffer == 0) {
                srsRecordTable.slots[i].buffer = smAlloc(SRS_SLOT_BUFFER_SIZE);
            }
            srsRecordTable.slots[i].mapPending = 1;
            srsRecordTable.slots[i].effectNo = slotTable->effectNo[i];
            size = fileLoad(srsRecordTable.slots[i].buffer, name, 1);
            if (size <= 0) {
                srsRecordTable.slots[i].mapPending = 0;
            } else if (size <= SRS_SLOT_BUFFER_SIZE) {
                srsRecordTable.slots[i].key = slotTable->effectNo[i] << 12;
            }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsLoadEffectBlock);
