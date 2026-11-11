#include "common.h"
#include "main/tu012.h"

/* xglMcRequest commands (the callers in tu060.c use the same values). */
#define MC_REQ_FREE_SPACE 1
#define MC_REQ_EXIST 2
#define MC_REQ_LIST 3
#define MC_REQ_LOAD 4
#define MC_REQ_SAVE 5
#define MC_REQ_CREATE 6
#define MC_REQ_FORMAT 7
#define MC_REQ_CHECK_OLD 8
#define MC_REQ_UNFORMAT 9
/* The request queue holds 16 entries. */
#define MC_QUEUE_MASK 0xF

/* Parameters of a request: the file number, its buffer and size, and a callback the list and
 * old-data scans call per entry (same layout as in tu060.c). */
typedef struct McRequestParam {
    int file;
    void *data;
    int size;
    int (*callback)(int);
} McRequestParam;

/* One queued request (0x20 bytes, xglMcQueue). */
typedef struct McQueueEntry {
    short port;               /* +0x00 */
    short command;            /* +0x02 */
    int *result;              /* +0x04 */
    int file;                 /* +0x08 */
    int data;                 /* +0x0C: the buffer address */
    int size;                 /* +0x10 */
    int (*callback)(int);     /* +0x14 */
    unsigned char unmodeled_18[8];
} McQueueEntry;

extern McQueueEntry xglMcQueue[];
extern unsigned char xglMcQueueEnd;
void xglMcMain(void);

/* Queues a memory-card request (the parameters are copied for the commands that use them),
 * clears *result and runs one step of xglMcMain. */
int xglMcRequest(int port, int command, McRequestParam *param, int *result)
{
    int index;
    McQueueEntry *entry;

    index = xglMcQueueEnd;
    *result = 0;
    entry = &xglMcQueue[index];
    entry->port = port;
    entry->result = result;
    entry->command = command;
    switch (command) {
    case MC_REQ_FREE_SPACE:
    case MC_REQ_FORMAT:
    case MC_REQ_UNFORMAT:
        break;
    case MC_REQ_EXIST:
    case MC_REQ_LIST:
    case MC_REQ_LOAD:
    case MC_REQ_SAVE:
    case MC_REQ_CREATE:
    case MC_REQ_CHECK_OLD:
        entry->file = param->file;
        entry->data = (int)param->data;
        entry->callback = param->callback;
        entry->size = param->size;
        break;
    }
    xglMcQueueEnd = (index + 1) & MC_QUEUE_MASK;
    xglMcMain();
    return 0;
}

#include "shared.h"

/* The icon.sys contents the create request keeps (sizeof(sceMcIconSys) bytes, byte aligned). */
typedef struct McIconSys {
    unsigned char bytes[0x3C4];
} McIconSys;

/* Memory-card state block at xglMcState. */
typedef struct XglMcState {
    unsigned char state;            /* +0x00: phase of the request state machine */
    unsigned char substate;         /* +0x01: step inside the current state, cleared per request */
    unsigned char unmodeled_02[6];
    int descriptor;                 /* +0x08: the open file */
    unsigned char unmodeled_0c[0xC];
    int create_index;               /* +0x18: progress of the create request */
    unsigned char unmodeled_1c[1];
    unsigned char selector;         /* +0x1D: selector for the card search */
    signed char card;               /* +0x1E: card index chosen by the search */
    unsigned char status;           /* +0x1F */
    char full_path[0x60];           /* +0x20: full path of the current request (xglMcBuildPath) */
    McQueueEntry request;           /* +0x80: the request being worked on; the card search reuses its
                                     * file number as the search mode (0..2) and data as the work address */
} XglMcState;

extern XglMcState xglMcState;
extern int D_00959F3C[];
extern signed char xglMcQueueTop;
extern unsigned char xglMcQueueEnd;
extern char D_0069DF70[];
extern char xglMcBasePath[];

/* Returns the status word of the card state (+0x04 of xglMcState). */
int xglMcGetState(void)
{
    return D_00959F3C[0];
}

/* Clears both queue indices and the request phase of the memory-card state. */
void xglMcReset(void)
{
    xglMcQueueTop = 0;
    xglMcState.state = 0;
    xglMcQueueEnd = 0;
}

/*
 * Writes decimal numbers into a save entry as full-width Shift-JIS digits (0x82 0x4F + digit).
 * value goes to object + 0x11B and 0x11D; when source is set, source[3] and source[2] go to object + 0x120..0x129.
 * A first value below 10 gets a full-width space (0x81 0x40) in front.
 */
void xglMcSetEntryNumbers(unsigned char *object, int value, const unsigned char *source)
{
    unsigned char *entry_text;
    unsigned char *source_text;
    int first_value;
    int second_value;
    signed char first_tens;

    entry_text = object + 0x40;
    if (value > 0)
    {
        entry_text[0xDD] = (signed char) ((value % 10) + 0x4F);
        entry_text[0xDB] = (signed char) (((value / 10) % 10) + 0x4F);
        if (source != 0)
        {
            first_value = source[3];
            source_text = object + 0x120;
            second_value = source[2];
            source_text[3] = (signed char) ((first_value % 10) + 0x4F);
            if (first_value < 10)
            {
                source_text[0] = -0x7F;
                source_text[1] = 0x40;
            }
            else
            {
                first_tens = (signed char) ((first_value / 10) + 0x4F);
                source_text[0] = -0x7E;
                source_text[1] = first_tens;
            }
            source_text[9] = (signed char) ((second_value % 10) + 0x4F);
            source_text[7] = (signed char) ((second_value / 10) + 0x4F);
        }
    }
}

/* Card numbers xglMcBuildPath does not print as digits. */
#define MC_CARD_NONE (-2)
#define MC_CARD_ANY (-1)
/* Selectors of xglMcBuildPath; other values point to a file name. */
#define MC_PATH_REPEAT 0
#define MC_PATH_END (-1)
#define MC_PATH_NAME_COUNT 6

/* File names of selectors 1..5: /icon.sys, /system.ico, /copy.ico, /delete.ico, /dummy.bin. */
extern const char *xglMcFileNames[MC_PATH_NAME_COUNT];

/* Builds the full path in xglMcState.full_path: base, then the card number as two digits ("*" for
 * MC_CARD_ANY, nothing for MC_CARD_NONE), then by selector the path so far once more, one of the
 * file names 1..5, nothing, or "/" and the name selector points to. Returns the path. */
char *xglMcBuildPath(const char *base, int card, int selector)
{
    char *dest;
    const char *source;
    const char *prefix_end;

    dest = xglMcState.full_path;
    source = base;
    while ((*dest = *source) != 0) {
        dest++;
        source++;
    }
    switch (card) {
    case MC_CARD_NONE:
        break;
    case MC_CARD_ANY:
        *dest = '*';
        dest++;
        break;
    default:
        dest[1] = '0' + card % 10;
        dest[0] = '0' + (card / 10) % 10;
        dest += 2;
        break;
    }
    switch (selector) {
    case MC_PATH_REPEAT:
        prefix_end = dest;
        for (source = xglMcState.full_path; source != prefix_end; source++) {
            *dest = *source;
            dest++;
        }
        *dest = 0;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        source = xglMcFileNames[selector];
        while ((*dest = *source) != 0) {
            dest++;
            source++;
        }
        break;
    case MC_PATH_END:
        *dest = 0;
        break;
    default:
        *dest = '/';
        dest++;
        source = (const char *)selector;
        while ((*dest = *source) != 0) {
            dest++;
            source++;
        }
        break;
    }
    return xglMcState.full_path;
}


/* Full path for card `card` (or -1 for the wildcard) with the suffix path, built on the base path xglMcBasePath. */
char *xglMcSetFullPath(int card, int selector)
{
    return xglMcBuildPath(xglMcBasePath, card, selector);
}

/* xglMcState.state while a request runs (set by xglMcExecute). */
#define MC_STATE_OPEN_CLOSE 2
#define MC_STATE_WRITE 3
#define MC_STATE_READ 4
#define MC_STATE_CREATE 5
#define MC_STATE_EXIST_SCAN 6
#define MC_STATE_LIST 7
#define MC_STATE_CHECK_OLD 8
/* sceMcOpen modes and the directory read size. */
#define MC_OPEN_READ 1
#define MC_OPEN_WRITE_CREATE 0x202
#define MC_DIRECTORY_MAX_ENTRIES 100

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
int sceMcMkdir(int port, int slot, const char *name);
int sceMcGetDir(int port, int slot, const char *name, int mode, int max_entries, int table);
int sceMcOpen(int port, int slot, const char *name, int mode);
extern char *xglMcBasePaths[];
int sceMcFormat(int port, int slot);
int sceMcUnformat(int port, int slot);

/* Starts the queued request: the SDK call for the command and the state xglMcMain then follows. */
void xglMcExecute(McQueueEntry *request)
{
    xglMcState.substate = 0;
    switch (request->command) {
    case MC_REQ_FREE_SPACE:
        sceMcGetInfo(request->port, 0, 0, request->result, 0);
        return;
    case MC_REQ_EXIST:
        if (request->file == -1) {
            xglMcSetFullPath(-1, -1);
            sceMcGetDir(request->port, 0, xglMcState.full_path, 0, MC_DIRECTORY_MAX_ENTRIES,
                        (request->data + 0xA3) & ~0x3F);
            xglMcState.state = MC_STATE_EXIST_SCAN;
            return;
        }
        xglMcSetFullPath(request->file, 0);
        sceMcOpen(request->port, 0, xglMcState.full_path, MC_OPEN_READ);
        xglMcState.state = MC_STATE_OPEN_CLOSE;
        return;
    case MC_REQ_LIST:
        xglMcSetFullPath(-1, -1);
        xglMcState.state = MC_STATE_LIST;
        sceMcGetDir(request->port, 0, xglMcState.full_path, 0, MC_DIRECTORY_MAX_ENTRIES,
                    (request->data + 0xA3) & ~0x3F);
        return;
    case MC_REQ_CHECK_OLD:
        xglMcBuildPath(xglMcBasePaths[0], -1, -1);
        request->file = 0;
        xglMcState.state = MC_STATE_CHECK_OLD;
        sceMcGetDir(request->port, 0, xglMcState.full_path, 0, MC_DIRECTORY_MAX_ENTRIES,
                    (request->data + 0xA3) & ~0x3F);
        return;
    case MC_REQ_CREATE:
        xglMcSetFullPath(request->file, -1);
        sceMcMkdir(request->port, 0, xglMcState.full_path);
        xglMcState.create_index = 0;
        xglMcState.state = MC_STATE_CREATE;
        return;
    case MC_REQ_SAVE:
        xglMcSetFullPath(request->file, 0);
        sceMcOpen(request->port, 0, xglMcState.full_path, MC_OPEN_WRITE_CREATE);
        xglMcState.state = MC_STATE_WRITE;
        return;
    case MC_REQ_LOAD:
        xglMcSetFullPath(request->file, 0);
        sceMcOpen(request->port, 0, xglMcState.full_path, MC_OPEN_READ);
        xglMcState.state = MC_STATE_READ;
        return;
    case MC_REQ_FORMAT:
        sceMcFormat(request->port, 0);
        return;
    case MC_REQ_UNFORMAT:
        sceMcUnformat(request->port, 0);
        break;
    }
}

/* The create request's buffer: a table of file offsets (file k spans offset[k]..offset[k + 1],
 * each start rounded up to 64 bytes, relative to the end of the rounded header size offset[0]),
 * then at +0x40 the icon.sys contents. */
typedef struct McCreateData {
    int offset[16];
    McIconSys icon;
} McCreateData;

/* Steps of the create request (xglMcState.substate). */
#define MC_CREATE_DELETE_SAVE 0
#define MC_CREATE_DELETE_DUMMY 1
#define MC_CREATE_COPY_ICON 2
#define MC_CREATE_LAST_OPTIONAL_CLOSE 6
#define MC_CREATE_DUMMY_WRITE 14
#define MC_CREATE_LAST_CLOSE 15
/* Each file takes three steps: open, write, close. */
#define MC_CREATE_STEPS_PER_FILE 3
/* The save folder is filled up to this many bytes by the dummy file. */
#define MC_CREATE_TOTAL_BYTES 0x42400
/* sceMc result the delete steps accept: the file did not exist. */
#define MC_RESULT_NO_ENTRY (-4)

/* The icon.sys contents the create request keeps (0x959FD8, right behind xglMcState). */
extern McIconSys xglMcIconSys;
int sceMcDelete(int port, int slot, const char *name);
int sceMcClose(int descriptor);
int sceMcWrite(int descriptor, const void *buffer, int size);

/* One step of the create request, called with the result of the previous card operation: deletes
 * the old save and dummy file, keeps the icon.sys contents, then opens, writes and closes the
 * files of the buffer's offset table and the dummy file that fills the folder. Returns 1 when
 * done, 0 while steps remain (the step count goes to the request's result), -1 on an error. */
int xglMcCreateStep(int result)
{
    int file;
    McCreateData *save;
    char *data;
    int *entry;
    int offset;
    int size;

    file = xglMcState.request.file;
    if (result >= 0 || (xglMcState.substate < 3 && result == MC_RESULT_NO_ENTRY)) {
        save = (McCreateData *)xglMcState.request.data;
        switch (xglMcState.substate) {
        case MC_CREATE_DELETE_SAVE:
            xglMcSetFullPath(file, 0);
            sceMcDelete(xglMcState.request.port, 0, xglMcState.full_path);
            break;
        case MC_CREATE_DELETE_DUMMY:
            xglMcSetFullPath(file, 5);
            sceMcDelete(xglMcState.request.port, 0, xglMcState.full_path);
            break;
        case MC_CREATE_COPY_ICON:
            xglMcIconSys = save->icon;
            break;
        case 3:
            break;
        case 6:
        case 9:
        case 12:
        case MC_CREATE_LAST_CLOSE:
            sceMcClose(xglMcState.descriptor);
            if (save->offset[3] == 0 && xglMcState.substate == MC_CREATE_LAST_OPTIONAL_CLOSE) {
                return 1;
            }
            if (xglMcState.substate == MC_CREATE_LAST_CLOSE) {
                return 1;
            }
            break;
        case 4:
        case 7:
        case 10:
        case 13:
            xglMcSetFullPath(file, xglMcState.substate / MC_CREATE_STEPS_PER_FILE + 1);
            sceMcOpen(xglMcState.request.port, 0, xglMcState.full_path, MC_OPEN_WRITE_CREATE);
            break;
        case 5:
        case 8:
        case 11:
            xglMcState.descriptor = result;
            data = (char *)save + ((save->offset[0] + 0x3F) & ~0x3F);
            /* The entry address is computed as an integer, index first (addu index*4, base). */
            entry = (int *)(xglMcState.substate / MC_CREATE_STEPS_PER_FILE * 4 + (int)save);
            offset = (entry[0] + 0x3F) & ~0x3F;
            size = entry[1] - offset;
            xglMcState.create_index += (size + 0x3FF) & ~0x3FF;
            sceMcWrite(result, data + offset, size);
            break;
        case MC_CREATE_DUMMY_WRITE:
            xglMcState.descriptor = result;
            sceMcWrite(result, save, MC_CREATE_TOTAL_BYTES - xglMcState.create_index);
            break;
        }
        xglMcState.substate++;
        *xglMcState.request.result = xglMcState.substate;
        return 0;
    }
    *xglMcState.request.result = result;
    return -1;
}

/* Entries in the table func_0019E408 searches. */
#define MC_SLOT_COUNT 100
#define MC_SLOT_WANTED 1 /* the value the search looks for */

/* Returns the index of the entry with value 1 in the 100-entry table nearest to index, searching
 * index, index+1, index-1, index+2, ...; -1 once both directions have run off the table. */
int func_0019E408(unsigned char *table, int index)
{
    int step;
    int misses;

    step = 1;
    misses = 0;
    while (table[index] != MC_SLOT_WANTED) {
        for (;;) {
            index += step;
            if (step > 0) {
                step = -step - 1;
            } else {
                step = 1 - step;
            }
            if ((unsigned int)index < MC_SLOT_COUNT) {
                break;
            }
            misses++;
            if (misses >= 2) {
                return -1;
            }
        }
        misses = 0;
    }
    return index;
}


/* Picks a card for the request, then starts a directory scan of 16 entries on it. Returns 1 when no card fits. */
int xglMcStartDirectoryScan(void)
{
    int card;

    card = func_0019E408((unsigned char *)xglMcState.request.data, xglMcState.selector);
    if (card < 0)
    {
        return 1;
    }
    xglMcState.card = (signed char) card;
    xglMcState.status = 0;
    xglMcSetFullPath(card, (int)D_0069DF70);
    sceMcGetDir(xglMcState.request.port, 0, xglMcState.full_path, 0, 0x10, (xglMcState.request.data + 0xA3) & ~0x3F);
    return 0;
}

/* Save folders of the card search, one per search mode (xglMcState.request.file): /BASLUS-20469Xen, /BISLPS-29005Xen, /BISLPS-25353. */
extern char *xglMcBasePaths[];
int sceMcOpen(int port, int slot, const char *name, int mode);

/* Opens the save file on the card nearest to the selector; when none fits, moves on to the next
 * search mode: mode 0 starts a directory read of up to 100 entries on the second base path
 * (returns -1), mode 1 opens the third base path, mode 2 gives up (returns 1). Returns 0 when a
 * file open was started. */
int func_0019E508(void)
{
    int card;

    card = func_0019E408((unsigned char *)xglMcState.request.data, xglMcState.selector);
    if (card < 0) {
        /* The search mode is tested unsigned (beqz for mode 0 after the test for 1). */
        switch ((unsigned int)xglMcState.request.file) {
        case 0:
            xglMcState.request.file = 1;
            xglMcBuildPath(xglMcBasePaths[xglMcState.request.file], -1, -1);
            sceMcGetDir(xglMcState.request.port, 0, xglMcState.full_path, 0, 100, (xglMcState.request.data + 0xA3) & ~0x3F);
            return -1;
        case 1:
            xglMcState.request.file = 2;
            xglMcBuildPath(xglMcBasePaths[xglMcState.request.file], -2, 0);
            sceMcOpen(xglMcState.request.port, 0, xglMcState.full_path, 1);
            return 0;
        case 2:
            return 1;
        }
    }
    xglMcState.card = card;
    xglMcState.status = 0;
    xglMcBuildPath(xglMcBasePaths[xglMcState.request.file], card, 0);
    sceMcOpen(xglMcState.request.port, 0, xglMcState.full_path, 1);
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu012", xglMcMain);
