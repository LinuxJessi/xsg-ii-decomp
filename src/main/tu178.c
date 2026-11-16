#include "common.h"
#include "main/sef.h"
#include "main/srs.h"
#include "main/sv_get_image_item.h"

/* srsRecordTable as this file indexes it (the SrsTable of srs.h split like
 * sef.c's id lookup view): the head record holds the effect package (image
 * mapper 0), the eight key records the pending-resource slots (mappers 1..8),
 * the slots the effect records (mappers 9..56). */
typedef struct SrsHeadTable {
    SrsRecord head;
    SrsRecord keyRecords[SRS_KEY_RECORD_COUNT - 1];
    SrsRecord slots[SRS_SLOT_COUNT + 1];
} SrsHeadTable;

extern SrsHeadTable srsRecordTable;

#define SRS_ESP_BUFFER_SIZE 0x28800  /* buffer of the effect package (record 0) */
#define SRS_SLOT_BUFFER_SIZE 0x1C000 /* buffer of a pending-resource slot */

/* Defined as void smAlloc(int) in sm_initilize.c; the allocation comes back in $v0. */
void *smAlloc(int size);
extern const char srsEspCfdefPath[];
extern const char srsEspDefaultPath[];
void srsMapPendingResources(void);

/* Loads an effect package file ("esp/cfdef.esp" when cfdef is set, else
 * "esp/default.esp") into the buffer of record 0 of srsRecordTable, allocating
 * it on first use. When the buffer already holds the package with the same key
 * and reload is 0, only its image mapper is registered again; otherwise the
 * file is read again. A failed first read clears the key and falls through
 * into that shared tail, so the file is read a second time. */
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
    if (srsRecordTable.head.handle == 0) {
        srsRecordTable.head.handle = smAlloc(SRS_ESP_BUFFER_SIZE);
        if (srsRecordTable.head.handle == 0) {
            return;
        }
        srsRecordTable.head.busy = 1;
        srsRecordTable.head.key = key;
        if (fileLoad(srsRecordTable.head.handle, path, 0) >= 0) {
            if (cfdef != 0) {
                xglCdWaitUntilIdle();
            }
            return;
        }
        srsRecordTable.head.busy = 0;
        srsRecordTable.head.key = 0;
        if (srsRecordTable.head.handle == 0) {
            return;
        }
    }
    svDeleteImageMapper(0);
    if (reload == 0 && srsRecordTable.head.key == key) {
        srsRecordTable.head.busy = 1;
        srsMapPendingResources();
        return;
    }
    srsRecordTable.head.busy = 1;
    srsRecordTable.head.key = key;
    if (fileLoad(srsRecordTable.head.handle, path, 0) >= 0) {
        if (cfdef != 0) {
            xglCdWaitUntilIdle();
        }
    } else {
        srsRecordTable.head.busy = 0;
        srsRecordTable.head.key = 0;
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
        keyIndex = (srsRecordTable.keyRecords[i].key >> 12) & 0x3FF;
        if (keyIndex != 0 && keyIndex == index) {
            if (srsRecordTable.keyRecords[i].handle != 0) {
                svDeleteImageMapper(i + 1);
                smFree(srsRecordTable.keyRecords[i].handle);
            }
            memset(&srsRecordTable.keyRecords[i], 0, sizeof(SrsRecord));
        }
    }
}

int GameResBlockFreeByBuffer(void *buffer);

/* Releases records 9..56 (image mapper i + 9): the buffer goes back to smFree
 * or GameResBlockFreeByBuffer by kind, then the record is cleared. */
void srsReleaseEffectRecords(void)
{
    int i;

    for (i = 0; i < 48; i++) {
        if (srsRecordTable.slots[i].handle != 0) {
            svDeleteImageMapper(i + 9);
            if (srsRecordTable.slots[i].kind == SRS_KIND_SM_BLOCK) {
                smFree(srsRecordTable.slots[i].handle);
            } else if (srsRecordTable.slots[i].kind == SRS_KIND_GAME_RES_BLOCK) {
                GameResBlockFreeByBuffer(srsRecordTable.slots[i].handle);
            }
            memset(&srsRecordTable.slots[i], 0, sizeof(SrsRecord));
        }
    }
}

/* Releases every pending-resource slot (records 1..8) that holds a buffer. */
void srsReleaseAllSlots(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (srsRecordTable.keyRecords[i].handle != 0) {
            svDeleteImageMapper(i + 1);
            smFree(srsRecordTable.keyRecords[i].handle);
            memset(&srsRecordTable.keyRecords[i], 0, sizeof(SrsRecord));
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
    if (srsRecordTable.head.handle != 0) {
        svDeleteImageMapper(0);
        smFree(srsRecordTable.head.handle);
        memset(&srsRecordTable.head, 0, sizeof(SrsRecord));
    }
}

/* Registers the image mapper of every loaded record still flagged
 * busy (image mapper still owed): the effect package (mapper 0, tag 3000), the pending-resource
 * slots (mappers 1..8, tag 2000, entry count from key bits 28..29) and the
 * effect records (mappers 9..56, tag 3600). */
void srsMapPendingResources(void)
{
    void *chunk[8]; /* svAnalyzeChunk reads only chunk[0]; the 0x20 bytes are the original frame */
    int i;

    if (srsRecordTable.head.handle != 0 && srsRecordTable.head.busy != 0) {
        chunk[0] = srsRecordTable.head.handle;
        svAddImageMapper(0, 0, chunk, 3000);
        srsRecordTable.head.busy = 0;
    }
    for (i = 0; i < 8; i++) {
        if (srsRecordTable.keyRecords[i].handle != 0 && srsRecordTable.keyRecords[i].busy != 0) {
            chunk[0] = srsRecordTable.keyRecords[i].handle;
            svAddImageMapper(i + 1, (srsRecordTable.keyRecords[i].key >> 28) & 3, chunk, 2000);
            srsRecordTable.keyRecords[i].busy = 0;
        }
    }
    for (i = 0; i < 48; i++) {
        if (srsRecordTable.slots[i].handle != 0 && srsRecordTable.slots[i].busy != 0) {
            chunk[0] = srsRecordTable.slots[i].handle;
            svAddImageMapper(i + 9, 0, chunk, 3600);
            srsRecordTable.slots[i].busy = 0;
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
 * buffer of pending-resource slot i, flags it busy, and sets
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
            if (srsRecordTable.keyRecords[i].handle == 0) {
                srsRecordTable.keyRecords[i].handle = smAlloc(SRS_SLOT_BUFFER_SIZE);
            }
            srsRecordTable.keyRecords[i].busy = 1;
            srsRecordTable.keyRecords[i].effectNo = slotTable->effectNo[i];
            size = fileLoad(srsRecordTable.keyRecords[i].handle, name, 1);
            if (size <= 0) {
                srsRecordTable.keyRecords[i].busy = 0;
            } else if (size <= SRS_SLOT_BUFFER_SIZE) {
                srsRecordTable.keyRecords[i].key = slotTable->effectNo[i] << 12;
            }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu178", srsLoadEffectBlock);
