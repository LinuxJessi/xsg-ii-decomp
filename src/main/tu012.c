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

/* Memory-card state block at xglMcState. The full path of the current request starts at +0x20. */
typedef struct XglMcState {
    unsigned char state;            /* +0x00: phase of the request state machine */
    unsigned char unmodeled_01[0x1C];
    unsigned char selector;         /* +0x1D: selector for the card search */
    signed char card;               /* +0x1E: card index chosen by the search */
    unsigned char status;           /* +0x1F */
    char full_path[0x60];           /* +0x20: full path of the current request (xglMcBuildPath) */
    short port;                     /* +0x80: sceMc port */
    unsigned char unmodeled_82[6];
    unsigned int mode;              /* +0x88: search mode of func_0019E508 (0..2) */
    int size;                       /* +0x8C: work pointer; sceMcGetDir reserves (size + 0xA3) & ~0x3F */
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

INCLUDE_ASM("asm/main/nonmatchings/tu012", xglMcExecute);

INCLUDE_ASM("asm/main/nonmatchings/tu012", xglMcCreateStep);

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

int func_001305B0();

/* Picks a card for the request, then starts a directory scan of 16 entries on it. Returns 1 when no card fits. */
int xglMcStartDirectoryScan(void)
{
    int card;

    card = func_0019E408((unsigned char *)xglMcState.size, xglMcState.selector);
    if (card < 0)
    {
        return 1;
    }
    xglMcState.card = (signed char) card;
    xglMcState.status = 0;
    xglMcSetFullPath(card, (int)D_0069DF70);
    func_001305B0(xglMcState.port, 0, xglMcState.full_path, 0, 0x10, (xglMcState.size + 0xA3) & ~0x3F);
    return 0;
}

/* Save folders of the card search, one per xglMcState.mode: /BASLUS-20469Xen, /BISLPS-29005Xen, /BISLPS-25353. */
extern char *xglMcBasePaths[];
int sceMcOpen(int port, int slot, const char *name, int mode);

/* Opens the save file on the card nearest to the selector; when none fits, moves on to the next
 * search mode: mode 0 starts a directory read of up to 100 entries on the second base path
 * (returns -1), mode 1 opens the third base path, mode 2 gives up (returns 1). Returns 0 when a
 * file open was started. */
int func_0019E508(void)
{
    int card;

    card = func_0019E408((unsigned char *)xglMcState.size, xglMcState.selector);
    if (card < 0) {
        switch (xglMcState.mode) {
        case 0:
            xglMcState.mode = 1;
            xglMcBuildPath(xglMcBasePaths[xglMcState.mode], -1, -1);
            func_001305B0(xglMcState.port, 0, xglMcState.full_path, 0, 100, (xglMcState.size + 0xA3) & ~0x3F);
            return -1;
        case 1:
            xglMcState.mode = 2;
            xglMcBuildPath(xglMcBasePaths[xglMcState.mode], -2, 0);
            sceMcOpen(xglMcState.port, 0, xglMcState.full_path, 1);
            return 0;
        case 2:
            return 1;
        }
    }
    xglMcState.card = card;
    xglMcState.status = 0;
    xglMcBuildPath(xglMcBasePaths[xglMcState.mode], card, 0);
    sceMcOpen(xglMcState.port, 0, xglMcState.full_path, 1);
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu012", xglMcMain);
