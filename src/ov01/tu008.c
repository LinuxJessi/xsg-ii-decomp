/*
 * OV01 original TU 8: 0x00a818e8..0x00a83d28 (53 functions)
 */
#include "common.h"
#include "main/tu021.h"
#include "main/tu092.h"
#include "ov01/tu008.h"
#include "ov01/tu009.h"
#include "ov01/tu023.h"
#include "ov01/tu070.h"

/* Sizes and counts of the data loader (OV01 TU 8). */
#define DATA_FILE_BUF_SIZE 0x2E800
#define DATA_THINK_BUF_SIZE 0x12000
#define DATA_STAT_BUF_SIZE 0x41000
#define DATA_FACE_BUF_SIZE 0x8000
#define DATA_BATTLE_SE_PRJ_SIZE 0x800
#define DATA_BATTLE_SE_VSF_SIZE 0x1E800
#define DATA_ARENA_SIZE 0x11800
#define DATA_STREAM_SIZE 0x200000
#define DATA_STREAM_SMALL_SIZE 0x2A800
#define DATA_UNIT_HEAP_LARGE 0x12cc00
#define DATA_UNIT_HEAP_SIZE 0xc8800
#define DATA_UNIT_HEAP_CACHED_PART 0x57800
#define DATA_ID_COUNT 141
#define DATA_SLOT_COUNT 11
#define DATA_STREAM_COUNT 2
#define DATA_BATTLE_ID_COUNT 16
#define DATA_OTHER_ID_COUNT (DATA_ID_COUNT - DATA_BATTLE_ID_COUNT)
#define DATA_CD_SECTOR_SIZE 2048

void dataFileBufAlloc(void);
int dataEntryBlockReset(void);
void *memset(void *, int, unsigned int);

struct DataFileRecord {
    int loaded_index;
    char *buffer;
    int size;
};

/* Two adjacent 0x0c-byte file records: the .prj and .vsf of a sound bank (the battle bank bts_se, and the voice of a slot). */
struct DataFileRecordPair {
    struct DataFileRecord prj;
    struct DataFileRecord vsf;
};

/* First two words of every file table (slot, stream entry, entry block): the cursor the next file is read to
 * (dataFileSlotLoad advances it) and the heap block the table owns (the same address, the heap block of ov01HeapBlockOf). */
struct DataFileHead {
    char *next_buffer;
    union {
        char *buffer;
    } base;
};

/* The four files of a model: pxy, txy, xhr, epf (0x30 bytes). */
struct DataModelFiles {
    struct DataFileRecord pxy;
    struct DataFileRecord txy;
    struct DataFileRecord xhr;
    struct DataFileRecord epf;
};

/* A motion file record and the 0x0c bytes after it that nothing reads. */
struct DataMotionFile {
    struct DataFileRecord file;
    unsigned char unmodeled_0C[0xC];
};

/* The voice files of a slot at +0x130: the prj record (loaded_index is the voice) and the vsf record. */
struct DataSlot {
    struct DataFileHead head;
    int id;
    int handle;
    /* Up to four models, one DataModelFiles from +0x10 (index 0 is the unit itself) and one motion file from +0xD0. */
    struct DataModelFiles models[4];
    struct DataMotionFile motions[4];
    struct DataFileRecordPair voice_files;
};

/* One map of a stream: the pxy and txy files (0x18 bytes). */
struct DataMapFiles {
    struct DataFileRecord pxy;
    struct DataFileRecord txy;
};

struct DataStreamEntry {
    struct DataFileHead head;
    int key;
    struct DataMapFiles maps[1];
};

extern struct DataSlot dataSlotPool[DATA_SLOT_COUNT];
extern struct DataStreamEntry dataStreamPool[DATA_STREAM_COUNT];
/* The entry block: the model entry files from +0x08 (pxy, txy, xhr, epf), the motion records at +0x38 and alt_index at +0x50. */
struct DataEntry {
    struct DataFileHead head;
    struct DataModelFiles model;
    struct DataFileRecord motions[2];
    int alt_index;
    unsigned char unmodeled_54[0x14];
};

extern struct DataEntry dataEntryBlock;

void dataFileInit(void) {
    memset(dataSlotPool, 0, sizeof(dataSlotPool));
    memset(dataStreamPool, 0, sizeof(dataStreamPool));
    memset(&dataEntryBlock, 0, sizeof(dataEntryBlock));
    ov01CdPendingReset();
    ov01HeapInit();
    dataFileBufAlloc();
    dataEntryBlockReset();
}

extern int D_00AC0DF4;

int dataStatBufGet(void)
{
    return D_00AC0DF4;
}

extern int dataFaceHandle;

int dataFaceHandleGet(void)
{
    return dataFaceHandle;
}


void dataFileBufLoad(struct DataFileRecord *file_record, char *path, int size) {
    char *buffer;

    buffer = ov01HeapAlloc(size);
    ov01CdReadFileAsync(path, buffer);
    file_record->buffer = buffer;
    file_record->size = size;
}

int xglCdGetFileSize(const char *path);

int dataFileSlotLoad(struct DataFileRecord *file_record, char *path, char **next_buffer, int pass)
{
    int size;

    if (pass == 0) {
        if (path != 0) {
            file_record->size = (xglCdGetFileSize(path) + (DATA_CD_SECTOR_SIZE - 1)) / DATA_CD_SECTOR_SIZE * DATA_CD_SECTOR_SIZE;
            size = file_record->size;
        } else {
            file_record->size = 0;
            size = 0;
        }
        return size;
    }
    if (file_record->size > 0) {
        file_record->buffer = *next_buffer;
        ov01CdReadFileAsync(path, file_record->buffer);
        *next_buffer += file_record->size;
    } else {
        file_record->buffer = 0;
    }
    return file_record->size;
}

extern char *dataBatDatPath;
extern int D_61D040;

void dataBatDatLoad(void) {
    ov01CdReadFileSync(dataBatDatPath, &D_61D040);
}

/* Word at 0x2B5DBC: the address of the scene data buffer (tu037 reads the same word as a key and declares it int). */
char *func_00A9B100(void);
int sprintf(char *, const char *, ...);
extern char dataPathBuf[];
extern char *dataThinkPath;
extern char dataThinkFmt[];
extern char *dataThinkJpPath;
extern char dataThinkJpFmt[];

struct GameWorkThinkView {
    unsigned char unmodeled_00[0x2C];
    char *think_buffer;
    unsigned char unmodeled_30[0xB8 - 0x30];
    char *think_data_end;
};

extern struct GameWorkThinkView D_2B5D90;
extern struct DataFileRecord D_00AC0E28;

void dataThinkScriptLoad(int index)
{
    char *buffer;
    int size;

    sprintf(dataPathBuf, dataThinkFmt, dataThinkPath, index);
    buffer = ov01HeapAlloc(DATA_THINK_BUF_SIZE);
    D_2B5D90.think_buffer = buffer;
    ov01CdReadFileSync(dataPathBuf, func_00A9B100());
    D_00AC0E28.buffer = buffer;
    D_00AC0E28.size = DATA_THINK_BUF_SIZE;
    size = (xglCdGetFileSize(dataPathBuf) + (DATA_CD_SECTOR_SIZE - 1)) / DATA_CD_SECTOR_SIZE * DATA_CD_SECTOR_SIZE;
    sprintf(dataPathBuf, dataThinkJpFmt, dataThinkJpPath, index);
    ov01CdReadFileSync(dataPathBuf, func_00A9B100() + size);
    D_2B5D90.think_data_end = func_00A9B100() + size;
}

extern char *dataStatPath;
extern struct DataFileRecord dataStatRecord;

void dataStatFileLoad(void) {
    dataFileBufLoad(&dataStatRecord, dataStatPath, DATA_STAT_BUF_SIZE);
}

extern char *dataFacePath;
extern struct DataFileRecord dataFaceRecord;

void dataFaceFileLoad(void) {
    int face_handle;

    face_handle = ov01LookupNonHeap(0x30041);
    if (face_handle != 0) {
        dataFaceHandle = face_handle;
        return;
    }
    dataFileBufLoad(&dataFaceRecord, dataFacePath, DATA_FACE_BUF_SIZE);
}

char *strcpy(char *, const char *);
char *strcat(char *, char *);
extern char *dataBattleSeName;
extern char *dataVoiceDirPath;
extern char *dataVsfExt;
extern char *dataPrjExt;
extern struct DataFileRecordPair dataBattleSeRecords;

void dataBattleSeLoad(void)
{
    strcpy(dataPathBuf, dataVoiceDirPath);
    strcat(dataPathBuf, dataBattleSeName);
    strcat(dataPathBuf, dataPrjExt);
    dataFileBufLoad(&dataBattleSeRecords.prj, dataPathBuf, DATA_BATTLE_SE_PRJ_SIZE);
    strcpy(dataPathBuf, dataVoiceDirPath);
    strcat(dataPathBuf, dataBattleSeName);
    strcat(dataPathBuf, dataVsfExt);
    dataFileBufLoad(&dataBattleSeRecords.vsf, dataPathBuf, DATA_BATTLE_SE_VSF_SIZE);
}


void dataBattleSeRelease(void) {
    seBankLoadMemory(2, dataBattleSeRecords.vsf.buffer, dataBattleSeRecords.prj.buffer, 0);
    ov01HeapFree(dataBattleSeRecords.prj.buffer);
    ov01HeapFree(dataBattleSeRecords.vsf.buffer);
}

void dataCdArenaOpen(void) {
    int arena_buffer;

    arena_buffer = ov01HeapAlloc(DATA_ARENA_SIZE);
    xglCdArenaOpen(arena_buffer, DATA_ARENA_SIZE);
}

extern char *dataPxyExt;
extern char *dataTxyExt;
extern char *dataXhrExt;
extern char *dataEpfExt;

int dataUnitFilesLoad(struct DataSlot *table, char *name, int index, int pass)
{
    int total;

    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataEpfExt);
    total = dataFileSlotLoad(&table->models[index].epf, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataXhrExt);
    total += dataFileSlotLoad(&table->models[index].xhr, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataTxyExt);
    total += dataFileSlotLoad(&table->models[index].txy, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataPxyExt);
    return total + dataFileSlotLoad(&table->models[index].pxy, dataPathBuf, &table->head.next_buffer, pass);
}

extern char *dataMotDirPath;
extern char *dataXapExt;

int dataMotFileLoad(struct DataSlot *table, char *name, int index, int pass)
{
    struct DataSlot **table_ref = &table;
    strcpy(dataPathBuf, dataMotDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataXapExt);
    return dataFileSlotLoad(&(*table_ref)->motions[index].file, dataPathBuf, &(*table_ref)->head.next_buffer, pass);
}

int dataVoiceFilesLoad(struct DataSlot *slot, char *name, int pass)
{
    char *voice_buffer;
    int prj_loaded;

    voice_buffer = 0;
    if (pass == 1) {
        voice_buffer = ov01HeapAlloc(slot->voice_files.prj.size + slot->voice_files.vsf.size);
    }
    strcpy(dataPathBuf, dataVoiceDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataPrjExt);
    prj_loaded = dataFileSlotLoad(&slot->voice_files.prj, dataPathBuf, &voice_buffer, pass);
    strcpy(dataPathBuf, dataVoiceDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataVsfExt);
    return prj_loaded + dataFileSlotLoad(&slot->voice_files.vsf, dataPathBuf, &voice_buffer, pass);
}

int handleAddressGet(int);

void dataSlotUnload(struct DataSlot *slot) {
    int handle_address;

    handle_address = handleAddressGet(slot->handle);
    seBankLoadMemory(slot->handle, slot->voice_files.vsf.buffer, slot->voice_files.prj.buffer, handle_address);
    handleStateSet(slot->handle);
    ov01HeapFree(slot->voice_files.prj.buffer);
}

extern char *dataMapDirPath;

int dataMapFileLoad(struct DataStreamEntry *table, char *name, int index, int pass)
{
    struct DataMapFiles *files;
    int first_loaded;

    strcpy(dataPathBuf, dataMapDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataPxyExt);
    files = &table->maps[index];
    first_loaded = dataFileSlotLoad(&files->pxy, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, dataMapDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataTxyExt);
    return first_loaded + dataFileSlotLoad(&files->txy, dataPathBuf, &table->head.next_buffer, pass);
}

int dataEntryFilesLoad(struct DataEntry *table, char *name, int index, int pass)
{
    int total;

    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataEpfExt);
    total = dataFileSlotLoad(&(&table->model)[index].epf, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataXhrExt);
    total += dataFileSlotLoad(&(&table->model)[index].xhr, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataTxyExt);
    total += dataFileSlotLoad(&(&table->model)[index].txy, dataPathBuf, &table->head.next_buffer, pass);
    strcpy(dataPathBuf, name);
    strcat(dataPathBuf, dataPxyExt);
    return total + dataFileSlotLoad(&(&table->model)[index].pxy, dataPathBuf, &table->head.next_buffer, pass);
}

void dataMotSlotLoad(struct DataEntry *table, char *name, int index, int pass)
{
    struct DataEntry **table_ref = &table;
    strcpy(dataPathBuf, dataMotDirPath);
    strcat(dataPathBuf, name);
    strcat(dataPathBuf, dataXapExt);
    dataFileSlotLoad(&(*table_ref)->motions[index], dataPathBuf, &(*table_ref)->head.next_buffer, pass);
}

int func_00A82368(struct DataEntry *entry, char *table_value, int which)
{
    return 0;
}

struct DataSlot *dataSlotFind(id)
int id;

{
    int i;

    for (i = 0; i < DATA_SLOT_COUNT; i++) {
        if (dataSlotPool[i].id == id) {
            return &dataSlotPool[i];
        }
    }
    return 0;
}

struct DataSlot *dataSlotAcquire(int id) {
    struct DataSlot *slot;
    int first;
    int limit;
    int i;

    slot = dataSlotFind(id);
    if (slot != 0) {
        return slot;
    }
    if ((unsigned int)(id - DATA_BATTLE_ID_COUNT) >= DATA_OTHER_ID_COUNT) {
        first = 0;
        limit = 8;
    } else {
        first = 8;
        limit = DATA_SLOT_COUNT;
    }
    i = first;
    while (i < limit) {
        if (dataSlotPool[i].id == 0) {
            slot = &dataSlotPool[i];
            slot->id = id;
            slot->head.next_buffer = 0;
            slot->head.base.buffer = 0;
            break;
        }
        i++;
    }
    return slot;
}

int dataSlotRelease(int id) {
    struct DataSlot *slot;

    slot = dataSlotFind(id);
    if (slot == 0) {
        return 0;
    }
    memset(slot, 0, sizeof(*slot));
    slot->id = 0;
    return 1;
}

int dataSlotFlagSet(int id) {
    struct DataSlot *slot;
    Ov01HeapBlock *block;

    slot = dataSlotFind(id);
    if (slot == 0) {
        return 0;
    }
    block = ov01HeapBlockOf(slot->head.base.buffer);
    block->flags |= 2;
    return 1;
}

int dataSlotFlagClear(int id) {
    struct DataSlot *slot;
    Ov01HeapBlock *block;

    slot = dataSlotFind(id);
    if (slot == 0) {
        return 0;
    }
    block = ov01HeapBlockOf(slot->head.base.buffer);
    block->flags &= ~2;
    return 1;
}

int dataSlotReclaim(int min_size) {
    Ov01HeapBlock *block;
    int i;

    for (i = 0; i < DATA_SLOT_COUNT; i++) {
        if (dataSlotPool[i].id != 0 && dataSlotPool[i].head.base.buffer != 0) {
            block = ov01HeapBlockOf(dataSlotPool[i].head.base.buffer);
            if ((block->flags & 3) == 1) {
                block = ov01HeapBlockOf(dataSlotPool[i].head.base.buffer);
                if (block->size >= min_size) {
                    ov01HeapFree(dataSlotPool[i].head.base.buffer);
                    memset(&dataSlotPool[i], 0, sizeof(dataSlotPool[i]));
                    dataSlotPool[i].id = 0;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int dataUnitLoad(int id)
{
    struct DataSlot *slot;
    int index;
    int size;
    int heap_size;
    int file_id;
    int model_id;

    slot = dataSlotAcquire(id);
    if (slot == 0) {
        return 0;
    }
    if (slot->head.next_buffer == 0) {
        if ((unsigned int)(id - DATA_BATTLE_ID_COUNT) >= DATA_OTHER_ID_COUNT) {
            heap_size = ((unsigned int)(id - 11) < 5) ? DATA_UNIT_HEAP_LARGE : DATA_UNIT_HEAP_SIZE;
            file_id = func_00A9B1B8(id);
            if (ov01LookupNonHeap(file_id) != 0) {
                heap_size -= DATA_UNIT_HEAP_CACHED_PART;
            }
            slot->head.next_buffer = ov01HeapAlloc(heap_size);
            slot->head.base.buffer = slot->head.next_buffer;
            if (slot->head.next_buffer == 0) {
                dataSlotReclaim(heap_size);
                slot->head.next_buffer = ov01HeapAlloc(heap_size);
                slot->head.base.buffer = slot->head.next_buffer;
            }
        }
    }
    if (id < DATA_BATTLE_ID_COUNT) {
        dataSlotFlagSet(id);
    }
    size = dataUnitFileIdLoad(slot, id, 0);
    size += dataMotSetLoad(slot, id, 0);
    index = 0;
    while (index < 3) {
        model_id = 0;
        if (id < DATA_BATTLE_ID_COUNT) {
            model_id = *(short *)((char *)battleSlotGet(id) + index * 2 + 116);
        }
        if (model_id != 0) {
            index++;
            size += dataModelLoad(slot, model_id, index);
            size += dataModelMotLoad(slot, model_id, index);
        } else {
            index++;
        }
    }
    dataVoiceSetLoad(slot, id);
    if ((((unsigned int)(id - 11) >= 130) && (size > DATA_UNIT_HEAP_SIZE)) || ((id < DATA_BATTLE_ID_COUNT) && (size > DATA_UNIT_HEAP_LARGE))) {
        for (;;) {
        }
    }
    return (int)slot;
}

int func_00A9B1B8(int value);
char *xglCdBuildFileName(char *buffer, unsigned int file_id);
extern char dataEntryPathBuf[];

/* Table offsets are plain ints here (the original computed base + offset for every record), so the slot is added to
 * them as a byte pointer: the typed slot is the parameter, `table` its byte view (typed spellings fold the constants). */
int dataUnitFileIdLoad(struct DataSlot *slot, int key, int index)
{
    char *table;
    int file_id;
    int stride_offset;
    int pxy_offset;
    int txy_offset;
    int xhr_offset;
    int epf_offset;
    struct DataFileRecord *record;
    char *base;
    char **cached_files;
    int total;
    void *heap_block;

    table = (char *)slot;

    file_id = func_00A9B1B8(key);
    stride_offset = index * 0x30;
    pxy_offset = stride_offset + 0x10;
    record = (struct DataFileRecord *)(table + pxy_offset);
    if (file_id == record->loaded_index) {
        return 0;
    }
    cached_files = (char **)ov01LookupNonHeap(file_id);
    if (cached_files != 0) {
        base = table + 4;
        txy_offset = stride_offset + 0x1C;
        xhr_offset = stride_offset + 0x28;
        epf_offset = stride_offset + 0x34;
        record->loaded_index = file_id;
        *(char **)(base + pxy_offset) = cached_files[0];
        *(char **)(base + txy_offset) = cached_files[1];
        *(char **)(base + xhr_offset) = cached_files[2];
        *(char **)(base + epf_offset) = cached_files[3];
        return 0;
    }
    xglCdBuildFileName(dataEntryPathBuf, file_id);
    record->loaded_index = file_id;
    total = dataUnitFilesLoad(slot, dataEntryPathBuf, index, 0);
    if ((unsigned int)(key - DATA_BATTLE_ID_COUNT) < DATA_OTHER_ID_COUNT) {
        heap_block = ov01HeapAlloc(total);
        slot->head.next_buffer = heap_block;
        slot->head.base.buffer = heap_block;
    }
    return dataUnitFilesLoad(slot, dataEntryPathBuf, index, 1);
}

int func_00A9B298(int value);
unsigned char *dataMotNameGet(int table_index);

/* The table is a DataSlot, but the record offset is an int added to its address (see dataUnitFileIdLoad). */
int dataMotSetLoad(struct DataSlot *slot, int id, int index)
{
    char *table;
    struct DataFileRecord *record;
    unsigned char *name;
    int name_index;
    int offset;
    int total_size;
    void *heap_block;

    table = (char *)slot;

    if (id >= DATA_ID_COUNT) {
        return 0;
    }
    name_index = func_00A9B298(id);
    offset = index * 0x18 + 0xD0;
    record = (struct DataFileRecord *)(table + offset);
    if (name_index == record->loaded_index) {
        return 0;
    }
    name = dataMotNameGet(name_index);
    record->loaded_index = name_index;
    total_size = dataMotFileLoad(slot, name, index, 0);
    if ((unsigned int)(id - DATA_BATTLE_ID_COUNT) < DATA_OTHER_ID_COUNT) {
        heap_block = ov01HeapAlloc(total_size);
        slot->head.next_buffer = heap_block;
        slot->head.base.buffer = heap_block;
    }
    return dataMotFileLoad(slot, name, index, 1);
}

int func_00A9B2A0(int index);
int handleAlloc(int flag, int value);
char *dataVoiceNameGet(int index);

int dataVoiceSetLoad(struct DataSlot *slot, int index) {
    int voice;
    char *name;

    if (index >= DATA_ID_COUNT) {
        return 0;
    }
    voice = func_00A9B2A0(index);
    if (voice == slot->voice_files.prj.loaded_index) {
        return 0;
    }
    name = dataVoiceNameGet(voice);
    slot->voice_files.prj.loaded_index = voice;
    slot->handle = handleAlloc(index >= DATA_BATTLE_ID_COUNT, dataVoiceFilesLoad(slot, name, 0));
    return dataVoiceFilesLoad(slot, name, 1);
}

void dataSlotReset(void) {
    struct DataSlot *slot;

    slot = dataSlotFind();
    if (slot != 0) {
        memset(&slot->voice_files, 0, sizeof(slot->voice_files));
        slot->handle = 0;
    }
}

extern char *dataModelDirPath;
unsigned char *dataModelNameGet(int table_index);
int dataModelKeyGet(int index);

/* Byte offsets into the slot: see dataUnitFileIdLoad. */
int dataModelLoad(struct DataSlot *slot, int id, int index)
{
    char *table;
    int stride_offset;
    int pxy_offset;
    struct DataFileRecord *record;
    char *base;
    char **cached_files;
    int txy_offset;
    int xhr_offset;
    int epf_offset;
    unsigned char *name;

    table = (char *)slot;

    stride_offset = index * 0x30;
    pxy_offset = stride_offset + 0x10;
    record = (struct DataFileRecord *)(table + pxy_offset);
    if (id == record->loaded_index) {
        return 0;
    }
    cached_files = (char **)ov01LookupNonHeap(dataModelKeyGet(id));
    if (cached_files != 0) {
        base = table + 4;
        txy_offset = stride_offset + 0x1C;
        xhr_offset = stride_offset + 0x28;
        epf_offset = stride_offset + 0x34;
        record->loaded_index = id;
        *(char **)(base + pxy_offset) = cached_files[0];
        *(char **)(base + txy_offset) = cached_files[1];
        *(char **)(base + xhr_offset) = cached_files[2];
        *(char **)(base + epf_offset) = cached_files[3];
        return 0;
    }
    name = dataModelNameGet(id);
    if (name == 0) {
        return 0;
    }
    strcpy(dataEntryPathBuf, dataModelDirPath);
    strcat(dataEntryPathBuf, name);
    record->loaded_index = id;
    dataUnitFilesLoad(slot, dataEntryPathBuf, index, 0);
    return dataUnitFilesLoad(slot, dataEntryPathBuf, index, 1);
}

int dataModelMotLoad(struct DataSlot *slot, int model_id, int index)
{
    char *table;
    struct DataFileRecord *record;
    unsigned char *name;
    int name_index;
    int offset;

    table = (char *)slot;

    offset = index * 0x18 + 0xD0;
    record = (struct DataFileRecord *)(table + offset);
    name_index = model_id + 143;
    if (name_index == record->loaded_index) {
        return 0;
    }
    name = dataMotNameGet(name_index);
    if (name == 0) {
        return 0;
    }
    record->loaded_index = name_index;
    dataMotFileLoad(slot, name, index, 0);
    return dataMotFileLoad(slot, name, index, 1);
}

struct SlotKeySource {
    unsigned char unmodeled_00[0x1E];
    short key;
};

struct SlotKeySource *func_00A9B2A8(int);
void sefAcquireSlot(int key);
void sefAcquireHighSlot(int key);

int unitEffectSlotAcquire(int key) {
    struct SlotKeySource *source;

    if (key >= DATA_ID_COUNT) {
        return 1;
    }
    if (key < DATA_BATTLE_ID_COUNT) {
        source = func_00A9B2A8(key);
        sefAcquireSlot(source->key);
        return 1;
    }
    source = func_00A9B2A8(key);
    sefAcquireHighSlot(source->key);
    return 1;
}

struct DataStreamEntry *dataStreamFind(key)
int key;

{
    int i;

    for (i = 0; i < DATA_STREAM_COUNT; i++) {
        if (dataStreamPool[i].key == key) {
            return &dataStreamPool[i];
        }
    }
    return 0;
}

struct DataStreamEntry *dataStreamAcquire(key)
int key;

{
    struct DataStreamEntry *entry;
    int i;

    entry = dataStreamFind(key);
    if (entry != 0) {
        return entry;
    }
    for (i = 0; i < DATA_STREAM_COUNT; i++) {
        if (dataStreamPool[i].key == 0) {
            entry = &dataStreamPool[i];
            entry->key = key;
            entry->head.next_buffer = 0;
            entry->head.base.buffer = 0;
            break;
        }
    }
    return entry;
}

int func_00A82F60(void) {
    if (dataStreamFind() != 0) {
        return 0;
    }
    /* The original passes a zero base address to the clear of 0x24 bytes. */
    memset(0, 0, 0x24);
    return 1;
}

int dataMapSetLoad(struct DataStreamEntry *buffer, int count, int index);
unsigned char *dataMapNameGet(int table_index);

struct DataStreamEntry *dataStreamOpen(int count, int small_buffer)
{
    struct DataStreamEntry *buffer;
    int buffer_size;

    buffer = dataStreamAcquire();
    if (buffer == 0) {
        return buffer;
    }
    buffer_size = DATA_STREAM_SIZE;
    if (small_buffer != 0) {
        buffer_size = DATA_STREAM_SMALL_SIZE;
    }
    if (buffer->head.next_buffer == 0) {
        buffer->head.next_buffer = ov01HeapAlloc(buffer_size);
        buffer->head.base.buffer = buffer->head.next_buffer;
    }
    if (buffer_size < dataMapSetLoad(buffer, count, 0)) {
        while (1) {
        }
    }
    return buffer;
}

int dataMapSetLoad(struct DataStreamEntry *buffer, int count, int index)
{
    struct DataMapFiles *maps;
    int *entry;
    char *name;

    maps = buffer->maps;
    entry = &maps[index].pxy.loaded_index;
    if (count == *entry) {
        return 0;
    }
    name = dataMapNameGet(count);
    *entry = count;
    dataMapFileLoad(buffer, name, index, 0);
    return dataMapFileLoad(buffer, name, index, 1);
}

struct DataEntry *dataEntryBlockGet(void)
{
    return &dataEntryBlock;
}

extern void *dataFileBufBase;

void dataFileBufAlloc(void) {
    dataFileBufBase = ov01HeapAlloc(DATA_FILE_BUF_SIZE);
}

int dataEntryBlockReset(void) {
    int *block;
    int preserved;

    block = (int *)&dataEntryBlock;
    preserved = block[1];
    memset(block, 0, sizeof(dataEntryBlock));
    block[1] = preserved;
    block[0] = preserved;
}


struct DataEntry *dataModelEntryLoad(int table_index) {
    struct DataEntry *entry;
    unsigned char *table_text;

    entry = dataEntryBlockGet();
    strcpy(dataEntryPathBuf, dataModelDirPath);
    table_text = dataModelNameGet(table_index);
    strcat(dataEntryPathBuf, table_text);
    entry->model.pxy.loaded_index = table_index;
    dataEntryFilesLoad(entry, dataEntryPathBuf, 0, 0);
    dataEntryFilesLoad(entry, dataEntryPathBuf, 0, 1);
    return entry;
}

struct DataMotSuffixPair {
    char *first;
    char *second;
};

extern struct DataMotSuffixPair D_00ABA1F0;

struct DataEntry *dataMotEntryLoad(int table_index, int index, int suffix_index)
{
    struct DataMotSuffixPair suffix;
    struct DataEntry *table;
    char *name;
    char **suffix_ptr;
    int *slot;

    suffix = D_00ABA1F0;
    table = dataEntryBlockGet();
    name = dataMotNameGet(table_index);
    if (199 <= table_index && table_index < 215) {
        strcpy(dataEntryPathBuf, name);
        suffix_ptr = &suffix.first + suffix_index;
        strcat(dataEntryPathBuf, *suffix_ptr);
        name = dataEntryPathBuf;
    }
    table->motions[index].loaded_index = table_index;
    dataMotSlotLoad(table, name, index, 0);
    dataMotSlotLoad(table, name, index, 1);
    return table;
}


struct DataEntry *dataVoiceEntryLoad(int table_index) {
    struct DataEntry *entry;
    char *table_value;

    entry = dataEntryBlockGet();
    table_value = dataVoiceNameGet(table_index);
    entry->alt_index = table_index;
    func_00A82368(entry, table_value, 0);
    func_00A82368(entry, table_value, 1);
    return entry;
}

extern unsigned char *dataModelNameTbl[];
extern unsigned char dataEmptyName[];

unsigned char *dataModelNameGet(int table_index) {
    unsigned char *text;

    text = dataModelNameTbl[table_index];
    if (text != 0) {
        return text;
    }
    return dataEmptyName;
}

extern unsigned char *dataMotNameTbl[];

unsigned char *dataMotNameGet(int table_index) {
    unsigned char *text;

    text = dataMotNameTbl[table_index];
    if (text != 0) {
        return text;
    }
    return dataEmptyName;
}

extern char *dataVoiceNameTbl[];

char *dataVoiceNameGet(int index)
{
    return dataVoiceNameTbl[index];
}

extern unsigned char *dataMapNameTbl[];

unsigned char *dataMapNameGet(int table_index) {
    unsigned char *text;

    text = dataMapNameTbl[table_index];
    if (text != 0) {
        return text;
    }
    return dataEmptyName;
}

extern char *dataBattleDirPath;
extern char *D_00AB4C60[];
extern char *D_00AB4F78[];
extern char D_00AB8FC8[];
extern char D_00AB8F98[];
int xglFlagsGet(int, int);

struct DataBattleState {
    unsigned char unmodeled_00[144];
    short one_based_index;
};

struct DataBattleOwner {
    unsigned char unmodeled_00[324];
    struct DataBattleState *state;
};

char *dataBattleCodePathGet(struct DataBattleOwner *owner, int code)
{
    int kind;
    int sp_index;
    int flags;

    if ((unsigned int)(code - 44) < 27) {
        if ((unsigned int)(code - 44) < 7) {
            kind = code - 44;
        } else if ((unsigned int)(code - 54) < 7) {
            kind = code - 47;
        } else if ((unsigned int)(code - 64) < 7) {
            kind = code - 50;
        } else {
            return 0;
        }
        strcpy(dataPathBuf, dataBattleDirPath);
        sp_index = owner->state->one_based_index - 1;
        if (sp_index >= 0) {
            flags = xglFlagsGet(401, 8);
            if (flags < 10 && code == 64) {
                strcat(dataPathBuf, D_00AB8FC8);
                return dataPathBuf;
            }
            if (flags < 10 && code == 67) {
                strcat(dataPathBuf, D_00AB8F98);
                return dataPathBuf;
            }
            strcat(dataPathBuf, D_00AB4F78[kind * 7 + sp_index]);
            return dataPathBuf;
        }
        return 0;
    }
    if ((unsigned int)(code - 71) < 10) {
        kind = code - 50;
    } else if ((unsigned int)(code - 81) < 16) {
        kind = code - 50;
    } else if ((unsigned int)(code - 631) < 40) {
        kind = code - 584;
    } else if (code >= 671) {
        kind = code - 584;
    } else {
        return 0;
    }
    strcpy(dataPathBuf, dataBattleDirPath);
    strcat(dataPathBuf, D_00AB4C60[kind]);
    return dataPathBuf;
}

int unitTblGet(int table, StatOwner ***units);
int ov01RollPercent(int percent);

extern char *dataBattleActionNameTbl[];
char *dataBattleRandomPathGet(int kind, int mode);

char *dataBattleActionPathGet(int kind, int mode)
{
    char *action_name;
    int mode_in_range;

    if ((unsigned int)(kind - 1) >= 10) {
        return 0;
    }
    mode_in_range = mode < 8;
    if (!mode_in_range) {
        return 0;
    }
    if ((unsigned int)(mode - 6) < 2) {
        action_name = dataBattleRandomPathGet(kind, mode);
    } else {
        action_name = dataBattleActionNameTbl[kind * 8 + mode];
    }
    strcpy(dataPathBuf, dataBattleDirPath);
    strcat(dataPathBuf, action_name);
    return dataPathBuf;
}

struct BattleNameSet {
    char *dir;
    int code[11];
};

extern struct BattleNameSet dataBattleBstSets[];
extern struct BattleNameSet dataBattleBedSets[];
extern char dataIntFmt[];
int rnd(int);
int battleCodeMatches(int kind, int code);

char *dataBattleRandomPathGet(int kind, int mode)
{
    struct BattleNameSet *name_set;
    int pick[11];
    char number_text[16];
    int i;
    int count;

    if (mode == 6) {
        name_set = dataBattleBstSets + (kind - 1);
    } else {
        name_set = dataBattleBedSets + (kind - 1);
    }
    count = 0;
    for (i = 10; i >= 0; i--) {
        if (battleCodeMatches(kind, name_set->code[i]) == 0) {
            continue;
        }
        if (name_set->code[i] >= 6) {
            pick[0] = i + 1;
            count = 1;
            break;
        }
        pick[count++] = i + 1;
    }
    count = pick[rnd(count - 1)];
    strcpy(dataEntryPathBuf, name_set->dir);
    sprintf(number_text, dataIntFmt, count);
    strcat(dataEntryPathBuf, number_text);
    return dataEntryPathBuf;
}

static inline StatOwner *unitSlotAt(StatOwner **table, int byteOffset)
{
    return *(StatOwner **)(byteOffset + (int)table);
}

int battleCodeMatches(int kind, int code)
{
    StatOwner **units;
    int count;
    int i;
    int hit;
    int slotOffset;
    int unitKind;

    switch (code) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
    case 3:
    case 4:
    case 5:
        count = unitTblGet(0, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];
            if (unit == 0) {
                continue;
            }
            unitKind = unit->stats->key;
            if (unitKind == 2 && code == 2) {
                return 1;
            }
            if (unitKind == 4 && code == 3) {
                return 1;
            }
            if (unitKind == 6 && code == 4) {
                return 1;
            }
            if (unitKind == 3 && code == 5) {
                return 1;
            }
        }
        return 0;
    case 6:
        count = unitTblGet(1, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];

            if (unit == 0) {
                continue;
            }
            if ((unit->stats->flags & 3) == 1) {
                return 1;
            }
        }
        return 0;
    case 7:
    case 8:
        count = unitTblGet(1, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];
            if (unit == 0) {
                continue;
            }
            unitKind = unit->stats->key;
            if ((unsigned int)(unitKind - 110) < 31) {
                if (code != 8) {
                    return 1;
                }
                if (ov01RollPercent(50) == 0) {
                    return 1;
                }
            }
        }
        return 0;
    case 9:
    case 10:
    case 11:
        count = unitTblGet(1, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];
            if (unit == 0) {
                continue;
            }
            unitKind = unit->stats->key;
            if (code == 9 && (unitKind == 110 || unitKind == 122)) {
                return 1;
            }
            if (code == 10 && unitKind == 126) {
                return 1;
            }
            if (code == 11 && (unitKind == 115 || unitKind == 127)) {
                return 1;
            }
        }
        return 0;
    case 12:
    case 13:
    case 14:
        count = unitTblGet(0, &units);
        i = 0;
        if (count > 0) {
            int sameKindCode = 12;
            int otherKindCode = 13;
            int fixedKindCode = 14;
            int fixedKind = 3;

            slotOffset = 0;
            do {
                if (unitSlotAt(units, slotOffset) != 0) {
                    if (code == sameKindCode && unitSlotAt(units, slotOffset)->stats->key == kind && partyUnitHpLow(unitSlotAt(units, slotOffset))) {
                        return 1;
                    }
                    if (code == otherKindCode && unitSlotAt(units, slotOffset)->stats->key != kind && partyUnitHpLow(unitSlotAt(units, slotOffset))) {
                        return 1;
                    }
                    if (code == fixedKindCode && unitSlotAt(units, slotOffset)->stats->key == fixedKind && partyUnitHpLow(unitSlotAt(units, slotOffset))) {
                        return 1;
                    }
                }
                i++;
                slotOffset += sizeof(StatOwner *);
            } while (i < count);
        }
        return 0;
    case 15:
        count = unitTblGet(1, &units);
        hit = 0;
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];

            if (unit == 0) {
                continue;
            }
            if ((unit->stats->flags & 3) == 1) {
                hit = 1;
                break;
            }
        }
        if (hit == 0) {
            return 0;
        }
        count = unitTblGet(0, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];

            if (unit == 0) {
                continue;
            }
            if (unit->stats->key == 2) {
                return 1;
            }
        }
        return 0;
    case 16:
        count = unitTblGet(1, &units);
        hit = 0;
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];
            if (unit == 0) {
                continue;
            }
            unitKind = unit->stats->key;
            if ((unsigned int)(unitKind - 110) < 31) {
                hit = 1;
                break;
            }
        }
        if (hit == 0) {
            return 0;
        }
        count = unitTblGet(0, &units);
        for (i = 0; i < count; i++) {
            StatOwner *unit = units[i];

            if (unit == 0) {
                continue;
            }
            if (unit->stats->key == 3) {
                return 1;
            }
        }
        return 0;
    case 17:
    case 18:
    case 19:
        if (code == 17 && ov01RollPercent(20) != 0) {
            return 1;
        }
        if (code == 18 && ov01RollPercent(15) != 0) {
            return 1;
        }
        if (code == 19 && ov01RollPercent(5) != 0) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}
