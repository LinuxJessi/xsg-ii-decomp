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

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF170);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF1B0);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF250);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF2D8);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF340);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF410);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF4C0);

#include "shared.h"

/* Completion callback of xglCdReadFile: slot index, buffer, state word, value. */
typedef void (*CdCallback)(int slot, void *buffer, int state, int value);

/* One 0x14-byte entry of the CD slot table xglCdSlotTable (xglCdSlotTable), indexed by slot.
 * buffer: destination of the read; tag: completion callback, 0 when idle;
 * value: word handed back to that callback (copied from xglCdSlotValue). */
typedef struct CdSlot {
    void *buffer;
    u8 unmodeled_04[8];
    int tag;
    int value;
} CdSlot;

extern int xglCdEventState;
extern CdSlot xglCdSlotTable[];

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

INCLUDE_ASM("asm/main/nonmatchings/tu054", xglCdFindFileGroup);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF678);

INCLUDE_ASM("asm/main/nonmatchings/tu054", xglCdFindFileId);

INCLUDE_ASM("asm/main/nonmatchings/tu054", xglCdBuildFileName);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF818);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF858);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF898);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF928);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF980);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DF9F0);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001DFA58);

extern int xglCdSlotValue;

/* Sets the value that xglCdReadFileTagged and xglCdReadFileSlot copy into the slot (xglCdSlotValue). */
void xglCdSetSlotValue(int value)
{
    xglCdSlotValue = value;
}

int xglCdGetFileSize(void);

int xglCdReadFileTagged(int file, int buffer, int unused_arg2, int tag) {
    int status;

    status = xglCdGetFileSize();
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

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0458);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0548);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0610);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0768);

INCLUDE_ASM("asm/main/nonmatchings/tu054", GameResLoadID);

INCLUDE_ASM("asm/main/nonmatchings/tu054", func_001E0AA8);
