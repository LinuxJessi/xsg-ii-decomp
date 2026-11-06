#include "common.h"
#include "main/xgl_cd.h"


/* Writes the low 16 bits of number as a 6-byte name: "cf" then four decimal digits, no NUL. */
void xglCdFormatFileName(char *buffer, int number)
{
    unsigned int remaining;
    int digit;

    remaining = number & 0xFFFF;
    buffer[0] = 'c';
    buffer[1] = 'f';
    for (digit = 0; digit < 4; digit++) {
        buffer[5 - digit] = remaining % 10U + '0';
        remaining = remaining / 10U;
    }
}

/* One 0x14-byte entry of the CD block table xglCdSlotTable, indexed by slot. The table
 * describes memory blocks in address order:
 *   buffer: block start; 0 ends the table (every search stops there).
 *   size:   block bytes (func_001DF410 splits, func_001DF4C0 merges by adding sizes).
 *   owner:  0 when free; a resource id, or CD_OWNER_NAMED plus bits of the buffer address
 *           for a block loaded by name (func_001DF898). Entry 0's owner is the first
 *           index the best-fit search func_001DF1B0 starts at.
 *   tag:    completion callback of a read into the block, 0 when idle.
 *   value:  word handed back to that callback (copied from xglCdSlotValue). */
typedef struct CdSlot {
    void *buffer;
    unsigned int size;
    int owner;
    int tag;
    int value;
} CdSlot;

/* Entries of xglCdSlotTable; every search walks indexes 1 .. CD_SLOT_COUNT - 1. */
#define CD_SLOT_COUNT 0x80
/* Block sizes are rounded up to this (func_001DF898). */
#define CD_BLOCK_ALIGN 0x40
/* A block loaded by name starts with the name in a header of this size (func_001E0768). */
#define CD_NAME_HEADER_SIZE 0x40
/* File sizes are rounded up to whole CD sectors. */
#define CD_SECTOR_SIZE 0x800
/* Owner tag of a block loaded by name, and the mask selecting that kind. */
#define CD_OWNER_NAMED 0xF00000
#define CD_OWNER_KIND_MASK 0xFF0000
/* Resource ids carry the file id in their low 24 bits (GameResLoadID). */
#define CD_FILE_ID_MASK 0xFFFFFF

extern CdSlot xglCdSlotTable[];
/* Address range D_0069E1A4 .. D_0069E1A8 whose blocks func_001DF250 and func_001DF340 skip. */
extern unsigned int D_0069E1A4;
extern unsigned int D_0069E1A8;

/* The name key stored in a block header: name itself when it fits the header; for longer
 * names the original returns name - (CD_NAME_HEADER_SIZE - 1), not an offset into name. */
char *func_001DF170(char *name)
{
    int length;

    for (length = 0; name[length] != 0; length++) {
    }
    if (length < CD_NAME_HEADER_SIZE) {
        return name;
    }
    return name - (CD_NAME_HEADER_SIZE - 1);
}

/* Best fit: the free block whose size equals size, else the smallest larger one; 0 when none. */
int func_001DF1B0(unsigned int size)
{
    unsigned int best_waste;
    int best;
    int index;

    best_waste = -1;
    best = 0;
    for (index = xglCdSlotTable[0].owner; index < CD_SLOT_COUNT && xglCdSlotTable[index].buffer != 0; index++) {
        if (xglCdSlotTable[index].owner == 0) {
            if (xglCdSlotTable[index].size == size) {
                return index;
            }
            if (size < xglCdSlotTable[index].size && xglCdSlotTable[index].size - size < best_waste) {
                best_waste = xglCdSlotTable[index].size - size;
                best = index;
            }
        }
    }
    return best;
}

/* The first block owned by owner whose buffer lies outside D_0069E1A4 .. D_0069E1A8. */
int func_001DF250(int owner)
{
    int index;

    if (owner == 0) {
        return 0;
    }
    for (index = 1; index < CD_SLOT_COUNT && xglCdSlotTable[index].buffer != 0; index++) {
        if (((unsigned int) xglCdSlotTable[index].buffer < D_0069E1A4
             || D_0069E1A8 < (unsigned int) xglCdSlotTable[index].buffer)
            && xglCdSlotTable[index].owner == owner) {
            return index;
        }
    }
    return 0;
}

/* The index of the block starting at buffer, or 0. */
int func_001DF2D8(void *buffer)
{
    int index;

    if (buffer == 0) {
        return 0;
    }
    for (index = 1; index < CD_SLOT_COUNT && xglCdSlotTable[index].buffer != 0; index++) {
        if (xglCdSlotTable[index].buffer == buffer) {
            return index;
        }
    }
    return 0;
}

/* The block outside D_0069E1A4 .. D_0069E1A8 that was loaded by name and whose header holds
 * the key func_001DF170 makes of name; 0 when none. */
int func_001DF340(char *name)
{
    char *key;
    char *wanted;
    char *stored;
    int index;

    key = func_001DF170(name);
    for (index = 1; index < CD_SLOT_COUNT && xglCdSlotTable[index].buffer != 0; index++) {
        if (((unsigned int) xglCdSlotTable[index].buffer < D_0069E1A4
             || D_0069E1A8 < (unsigned int) xglCdSlotTable[index].buffer)
            && (xglCdSlotTable[index].owner & CD_OWNER_KIND_MASK) == CD_OWNER_NAMED) {
            wanted = key;
            stored = xglCdSlotTable[index].buffer;
            while (*wanted == *stored) {
                if (*wanted == 0) {
                    return index;
                }
                wanted++;
                stored++;
            }
        }
    }
    return 0;
}

/* Gives block index to owner, first splitting off the bytes beyond size as a new free block. */
void *func_001DF410(int index, int owner, unsigned int size)
{
    CdSlot *entry;
    int i;

    entry = &xglCdSlotTable[index];
    if (entry->size != size) {
        for (i = CD_SLOT_COUNT - 2; index < i; i--) {
            xglCdSlotTable[i + 1].buffer = xglCdSlotTable[i].buffer;
            xglCdSlotTable[i + 1].size = xglCdSlotTable[i].size;
            xglCdSlotTable[i + 1].owner = xglCdSlotTable[i].owner;
            xglCdSlotTable[i + 1].tag = xglCdSlotTable[i].tag;
            xglCdSlotTable[i + 1].value = xglCdSlotTable[i].value;
        }
        entry[1].size = entry->size - size;
        entry->size = size;
        entry[1].buffer = (char *) entry->buffer + size;
        entry[1].owner = 0;
    }
    entry->owner = owner;
    return entry->buffer;
}

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF4C0);

#include "shared.h"

/* Completion callback of xglCdReadFile: slot index, buffer, state word, value. */
typedef void (*CdCallback)(int slot, void *buffer, int state, int value);

extern int xglCdEventState;

/* Completion handler passed to xglCdReadFile (proposed xglCdSlotEvent): event 1 stores value as the
 * state word xglCdEventState (xglCdEventState); event 4 calls the slot's callback with that state word. */
void xglCdSlotEvent(int event, int value, int slot) {
    CdSlot *entry;
    CdCallback callback;

    entry = &xglCdSlotTable[slot];
    callback = (CdCallback) entry->tag;
    if (callback != 0) {
        if (event == 1) {
            xglCdEventState = value;
        } else if (event == 4) {
            callback(slot, entry->buffer, xglCdEventState, entry->value);
        }
    }
}

/* One 0xC-byte entry of xglCdFileGroups: file ids base .. base + count - 1 map to
 * names[0 .. count - 1] (xglCdFindFileId, func_001DF678); xglCdBuildFileName also writes
 * names[0] in front of the file's own name. An entry with no names ends the table. */
typedef struct CdFileGroup {
    unsigned int base;
    unsigned int count;
    char **names;
} CdFileGroup;

extern CdFileGroup xglCdFileGroups[];
extern int strcmp(const char *a, const char *b);

void func_001DF4C0(int index);

/* Returns the group whose id range holds file_id, or 0 when none does. */
CdFileGroup *xglCdFindFileGroup(unsigned int file_id)
{
    CdFileGroup *group;

    for (group = xglCdFileGroups; group->names != 0; group++) {
        if (file_id >= group->base && file_id < group->base + group->count) {
            return group;
        }
    }
    return 0;
}

/* The name of file_id within its group, or 0 when no group holds it. */
char *func_001DF678(unsigned int file_id)
{
    CdFileGroup *group;

    group = xglCdFindFileGroup(file_id);
    if (group == 0) {
        return 0;
    }
    return group->names[file_id - group->base];
}

/* Returns the file id whose name is name, or -1 when no group lists it. */
int xglCdFindFileId(char *name)
{
    CdFileGroup *group;
    unsigned int index;

    for (group = xglCdFileGroups; group->names != 0; group++) {
        for (index = 0; index < group->count; index++) {
            if (strcmp(group->names[index], name) == 0) {
                return index + group->base;
            }
        }
    }
    return -1;
}

/* Writes the group's names[0] and then file_id's name to buffer; returns the end of the
 * written name (its NUL), or buffer unchanged when no group holds file_id. */
char *xglCdBuildFileName(char *buffer, unsigned int file_id)
{
    CdFileGroup *group;
    char **names;
    char *out;
    char *source;

    group = xglCdFindFileGroup(file_id);
    out = buffer;
    if (group != 0) {
        names = group->names;
        for (source = names[0]; *source != 0; source++) {
            *out++ = *source;
        }
        source = names[file_id - group->base];
        while ((*out++ = *source++) != 0) {
        }
        return out - 1;
    }
    return buffer;
}

/* Frees the block starting at buffer; returns its former index, 0 when no block starts there. */
int func_001DF818(void *buffer)
{
    int index;

    index = func_001DF2D8(buffer);
    if (index == 0) {
        return 0;
    }
    func_001DF4C0(index);
    return index;
}

/* Frees the block of resource owner; returns its former index, 0 when none holds it. */
int func_001DF858(int owner)
{
    int index;

    index = func_001DF250(owner);
    if (index == 0) {
        return 0;
    }
    func_001DF4C0(index);
    return index;
}

/* Allocates a block of size bytes (rounded up to CD_BLOCK_ALIGN) for owner; owner 0 makes
 * a named-file tag from the block's address. Returns the block index, 0 when none fits. */
int func_001DF898(int owner, int size)
{
    int index;

    size = (size + (CD_BLOCK_ALIGN - 1)) & ~(CD_BLOCK_ALIGN - 1);
    index = func_001DF1B0(size);
    if (index == 0) {
        return 0;
    }
    if (owner == 0) {
        owner = CD_OWNER_NAMED + (((unsigned int) xglCdSlotTable[index].buffer >> 8) & 0xFFFF);
    }
    func_001DF410(index, owner, size);
    return index;
}

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF928);

/* The largest free block, 0 when none is free. */
int func_001DF980(void)
{
    unsigned int best_size;
    int best;
    int index;

    best_size = 0;
    best = 0;
    for (index = 1; index < CD_SLOT_COUNT && xglCdSlotTable[index].buffer != 0; index++) {
        if (xglCdSlotTable[index].owner == 0 && best_size < xglCdSlotTable[index].size) {
            best_size = xglCdSlotTable[index].size;
            best = index;
        }
    }
    return best;
}

/* Buffer and size of the largest free block (entry 0's when none is free). */
void *func_001DF9F0(unsigned int *size)
{
    int index;

    index = func_001DF980();
    if (size != 0) {
        *size = xglCdSlotTable[index].size;
    }
    return xglCdSlotTable[index].buffer;
}

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DFA58);

/* Called by GameResLoadID and func_001E0768 for a block that was already loaded:
 * block index, buffer, size and xglCdSlotValue. Stored as a block's read tag otherwise. */
typedef void (*CdLoadCallback)(int index, void *buffer, unsigned int size, int value);

extern int xglCdSlotValue;

/* Sets the value that xglCdReadFileTagged and xglCdReadFileSlot copy into the slot (xglCdSlotValue). */
void xglCdSetSlotValue(int value)
{
    xglCdSlotValue = value;
}

int xglCdGetFileSize(const char *name);

int xglCdReadFileTagged(int file, int buffer, int unused_arg2, int tag) {
    int status;

    status = xglCdGetFileSize((const char *) file);
    if (status > 0) {
        if (buffer != 0) {
            xglCdSlotTable[0].tag = tag;
            xglCdSlotTable[0].value = xglCdSlotValue;
            xglCdSetReadTag(0);
            xglCdReadFile(file, buffer, 1, xglCdSlotEvent);
        }
    }
    return status;
}

/* Slot index to buffer (xglCdSlotGetBuffer): an index below 0x80 returns that entry's buffer; others pass through. */
unsigned int xglCdSlotGetBuffer(unsigned int index)
{
    unsigned int result;

    result = index;
    if (result != 0) {
        if (result < 0x80U) {
            result = (unsigned int) xglCdSlotTable[result].buffer;
        }
    }
    return result;
}

extern u8 D_0069E1BC;

/* Starts a read into a slot (proposed xglCdReadFileSlot). tag 0 reads without a slot;
 * otherwise the slot (index 0 when slot >= 0x80) takes tag and xglCdSlotValue before xglCdReadFile. */
void xglCdReadFileSlot(int file, int buffer, int tag, u32 slot) {
    CdSlot *entry;
    u32 index;
    int in_table;
    int value;

    in_table = slot < 0x80U;
    if (tag == 0) {
        if (D_0069E1BC != 0) {
            xglCdReadFile(file, buffer, 1, 0);
            return;
        }
        xglCdReadFile(file, buffer, tag, 0);
        return;
    }
    index = 0;
    entry = &xglCdSlotTable[0];
    if (in_table != 0) {
        index = slot;
        entry = &xglCdSlotTable[index];
    }
    value = xglCdSlotValue;
    entry->tag = tag;
    entry->value = value;
    xglCdSetReadTag(index);
    if (D_0069E1BC != 0) {
        xglCdReadFile(file, buffer, 1, xglCdSlotEvent);
        return;
    }
    xglCdReadFile(file, buffer, 0, xglCdSlotEvent);
}

int xglCdCheckFile(int, unsigned int *);

/* Writes the four-character extension ".xap" with its NUL, then calls xglCdCheckFile on first (proposed xglCdSetXapExt; the second call's role is open). */
int xglCdCheckXapFile(int first, char *extension)
{
    unsigned int work[0xC];

    extension[0] = '.';
    extension[1] = 'x';
    extension[2] = 'a';
    extension[3] = 'p';
    extension[4] = 0;
    return xglCdCheckFile(first, work);
}

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DFCA8);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DFD20);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E00F0);

/* Reads file_id as ".xap" (or ".fap" when no ".xap" exists) into slot's buffer; returns the
 * bytes it takes rounded up to CD_SECTOR_SIZE, 0 when the file is missing. */
int func_001E0458(unsigned int file_id, unsigned int slot, int tag)
{
    char name[256];
    char *extension;
    unsigned int buffer;
    unsigned int start;
    int size;

    buffer = xglCdSlotGetBuffer(slot);
    start = buffer;
    extension = xglCdBuildFileName(name, file_id);
    extension[0] = '.';
    extension[1] = 'x';
    extension[2] = 'a';
    extension[3] = 'p';
    extension[4] = 0;
    size = xglCdGetFileSize(name);
    if (size < 0) {
        extension[1] = 'f';
        size = xglCdGetFileSize(name);
    }
    if (size > 0) {
        if (slot != 0) {
            xglCdReadFileSlot((int) name, buffer, tag, slot);
        }
        buffer += (size + (CD_SECTOR_SIZE - 1)) & ~(CD_SECTOR_SIZE - 1);
    }
    return buffer - start;
}

/* Same for ".ene" files, with no fallback extension. */
int func_001E0548(unsigned int file_id, unsigned int slot, int tag)
{
    char name[256];
    char *extension;
    unsigned int buffer;
    unsigned int start;
    int size;

    buffer = xglCdSlotGetBuffer(slot);
    start = buffer;
    extension = xglCdBuildFileName(name, file_id);
    extension[0] = '.';
    extension[1] = 'e';
    extension[2] = 'n';
    extension[3] = 'e';
    extension[4] = 0;
    size = xglCdGetFileSize(name);
    if (size > 0) {
        if (slot != 0) {
            xglCdReadFileSlot((int) name, buffer, tag, slot);
        }
        buffer += (size + (CD_SECTOR_SIZE - 1)) & ~(CD_SECTOR_SIZE - 1);
    }
    return buffer - start;
}

int func_001DFD20(unsigned int file_id, unsigned int slot, int tag);
int func_001E00F0(unsigned int file_id, unsigned int slot, int tag);
int func_001E0610(int id, unsigned int slot, int *size, int tag);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0610);

/* Loads the file name into a block whose CD_NAME_HEADER_SIZE header holds the name, unless a block
 * already holds it. Reports the data (past the header) and its size; returns the owner tag,
 * -1 when the file is missing or no block is free. */
int func_001E0768(char *name, void **data, unsigned int *size, CdLoadCallback callback)
{
    int index;
    int bytes;
    char *buffer;
    char *source;
    char *header;

    index = func_001DF340(name);
    if (index > 0) {
        buffer = xglCdSlotTable[index].buffer;
        buffer += CD_NAME_HEADER_SIZE;
        if (data != 0) {
            *data = buffer;
        }
        if (size != 0) {
            *size = xglCdSlotTable[index].size;
        }
        if (callback != 0) {
            callback(index, xglCdSlotTable[index].buffer, xglCdSlotTable[index].size, xglCdSlotValue);
        }
        return xglCdSlotTable[index].owner;
    }
    bytes = xglCdGetFileSize(name);
    if (bytes < 0) {
        return -1;
    }
    bytes = (bytes + (CD_SECTOR_SIZE - 1)) & ~(CD_SECTOR_SIZE - 1);
    bytes += CD_NAME_HEADER_SIZE;
    index = func_001DF898(0, bytes);
    if (index == 0) {
        return -1;
    }
    buffer = xglCdSlotTable[index].buffer;
    source = func_001DF170(name);
    header = buffer;
    while ((*header++ = *source++) != 0) {
    }
    xglCdSlotTable[index].tag = (int) callback;
    xglCdSlotTable[index].value = xglCdSlotValue;
    buffer += CD_NAME_HEADER_SIZE;
    if (data != 0) {
        *data = buffer;
    }
    if (size != 0) {
        *size = bytes;
    }
    xglCdSetReadTag(index);
    xglCdReadFile(name, buffer, 1, (int) xglCdSlotEvent);
    return xglCdSlotTable[index].owner;
}

/* Returns the owner tag of the block holding resource id, loading it first when no block
 * holds it (-1: no such file, -2: no free block). Reports buffer and size, and calls
 * callback with the block when it was already loaded. */
int GameResLoadID(int id, void **buffer, unsigned int *size, CdLoadCallback callback)
{
    unsigned int file_id;
    int index;
    int total;

    file_id = id & CD_FILE_ID_MASK;
    index = func_001DF250(file_id);
    if (index != 0) {
        if (buffer != 0) {
            *buffer = xglCdSlotTable[index].buffer;
        }
        if (size != 0) {
            *size = xglCdSlotTable[index].size;
        }
        if (callback != 0) {
            callback(index, xglCdSlotTable[index].buffer, xglCdSlotTable[index].size, xglCdSlotValue);
        }
        return xglCdSlotTable[index].owner;
    }
    total = func_001E0610(id, 0, 0, 0);
    if (total == 0) {
        return -1;
    }
    index = func_001DF898(file_id, total);
    if (index == 0) {
        return -2;
    }
    func_001E0610(id, index, size, (int) callback);
    if (buffer != 0) {
        *buffer = xglCdSlotTable[index].buffer;
    }
    return xglCdSlotTable[index].owner;
}

/* The data (past the CD_NAME_HEADER_SIZE header) of the block loaded by name, and its size; 0 when absent. */
void *func_001E0AA8(char *name, unsigned int *size)
{
    int index;
    char *data;

    index = func_001DF340(name);
    if (index > 0) {
        data = xglCdSlotTable[index].buffer;
        data += CD_NAME_HEADER_SIZE;
        if (size != 0) {
            *size = xglCdSlotTable[index].size;
        }
        return data;
    }
    return 0;
}
