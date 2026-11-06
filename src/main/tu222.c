#include "common.h"
#include "shared.h"
#include "main/tu229.h"

extern void *memset(void *destination, int value, unsigned int count);

/* One 0x98-byte record of the message window (main/tu148), also the choice
 * and panel records that msgReset clears. */
typedef struct MsgEntry {
    u8 unmodeled_00[0x98];
} MsgEntry;

/* The message window: word 0 is the index of the current record, -1 for none;
 * only the first five records are modelled here. */
typedef struct MsgWindow {
    int current_index;
    u8 unmodeled_04[4];
    MsgEntry entries[1];
} MsgWindow;

void func_0027D878(MsgEntry *entry);
void msgEntryScrollLines(MsgEntry *entry, int value);
void msgEntrySeekToLimit(MsgEntry *entry);
void talkSeekToWindowEnd(void *entry);
void interpRewind(void *entry);
void talkSetSetting(void *entry, int value);
void msgWindowMoveCursor(MsgEntry *entry, int value);
void msgWindowClose(MsgEntry *entry);
int func_00286CA0(int *index, MsgWindow *window, int mode);

void func_0027D438(MsgWindow *window) {
    if (window->current_index >= 0) {
        func_0027D878(&window->entries[window->current_index]);
    }
}

void msgWindowScrollLines(MsgWindow *window, int value) {
    if (window->current_index >= 0) {
        msgEntryScrollLines(&window->entries[window->current_index], value);
    }
}

void msgWindowSeekToLimit(MsgWindow *window) {
    if (window->current_index >= 0) {
        msgEntrySeekToLimit(&window->entries[window->current_index]);
    }
}

void msgWindowSeekToWindowEnd(MsgWindow *window) {
    if (window->current_index >= 0) {
        talkSeekToWindowEnd(&window->entries[window->current_index]);
    }
}

void msgWindowRewindLastState1(MsgWindow *window) {
    int index;

    if (func_00286CA0(&index, window, 1) != 0) {
        interpRewind(&window->entries[index]);
    }
}

void msgWindowSetSetting(MsgWindow *window, int value) {
    if (window->current_index >= 0) {
        talkSetSetting(&window->entries[window->current_index], value);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu222", msgWindowPollCurrent);

void msgWindowMoveSelection(MsgWindow *window, int value) {
    if (window->current_index >= 0) {
        msgWindowMoveCursor(&window->entries[window->current_index], value);
    }
}

void msgWindowCloseLastState2(MsgWindow *window) {
    int index;

    if (func_00286CA0(&index, window, 2) != 0) {
        msgWindowClose(&window->entries[index]);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu222", msgWindowTestCurrentFlags);

INCLUDE_ASM("asm/main/nonmatchings/tu222", msgWindowAllInactive);

INCLUDE_ASM("asm/main/nonmatchings/tu222", msgWindowCurrentStatus);

void talkSetFlag10(void *entry);

void func_0027D878(MsgEntry *entry) {
    talkSetFlag10(entry);
}

void talkScrollLines(void *entry, int value);

void msgEntryScrollLines(MsgEntry *entry, int value) {
    talkScrollLines(entry, value);
}

void talkSeekToLimit(void *entry);

void msgEntrySeekToLimit(MsgEntry *entry) {
    talkSeekToLimit(entry);
}


void msgEntryClear(MsgEntry *entry) {
    memset(entry, 0, sizeof(MsgEntry));
    talkStartScriptLoad();
}

void msgQueueSlot();

void msgQueueEntry(void) {
    msgQueueSlot();
}


void msgEntryClearEndScript(MsgEntry *entry) {
    memset(entry, 0, sizeof(MsgEntry));
    talkEndScript();
}
