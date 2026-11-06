#include "common.h"
#include "shared.h"
#include "main/tu114.h"
#include "main/tu147.h"
#include "main/tu148.h"
#include "main/tu214.h"
#include "main/tu216.h"
#include "main/tu217.h"
#include "main/tu220.h"
#include "main/tu223.h"

/* Message-system state. msgWindow is the window object, msgChoice the choice
 * object and msgPanel the panel object; the init, close and show functions
 * below call the same object methods on each of them. */
extern int msgSlotIndex;   /* current slot index into msgSlotFlags; -1 when none */
extern int msgChoiceTimer;   /* choice timer: set to 0x5A, ticked, cleared at zero */
extern int msgSlotFlags[5]; /* five per-slot flag words */
extern unsigned char msgChoice[];
extern unsigned char msgWindow[];
extern unsigned char msgPanel[];
extern unsigned char PadData[];
void *memset(void *, int, unsigned int);
void msgWindowReset(void *);
void msgWindowRunAll(void *);
void msgEntryClear(void *);

/* Reset the slot flags, the window, the choice and the slot index. */
void msgStateReset(void) {
    memset(msgSlotFlags, 0, 0x14);
    msgWindowReset(msgWindow);
    msgWindowRunAll(msgWindow);
    msgWindowSetStartValue(1);
    msgEntryClear(msgChoice);
    msgSlotIndex = -1;
    msgChoiceTimer = 0;
    func_0022DEE0();
}

/* Same reset as msgStateReset, and the panel object is initialised too. */
void msgResetAll(void) {
    memset(msgSlotFlags, 0, 0x14);
    msgWindowReset(msgWindow);
    msgWindowRunAll(msgWindow);
    msgWindowSetStartValue(1);
    msgEntryClear(msgChoice);
    msgEntryClear(msgPanel);
    msgSlotIndex = -1;
    msgChoiceTimer = 0;
    func_0022DEE0();
}

int msgSlotTestCurrent(int mask);

void func_002512B0(void) {
    if (msgSlotTestCurrent(4) == 0) {
        msgGameFlagClear();
    }
}

int msgSlotTest(int index, int mask);
int msgSlotCurrent(void);

/* Test the flag bits of the current slot. */
int msgSlotTestCurrent(int mask) {
    return msgSlotTest(msgSlotCurrent(), mask);
}

/* Clear every slot flag word. */
void msgSlotClearAll(void) {
    int count;

    for (count = 4; count >= 0; count--) {
        msgSlotFlags[count] = 0;
    }
}

/* Store a flag word for one slot. */
void msgSlotSet(int index, int value) {
    msgSlotFlags[index] = value;
}

/* Return the bits of slot `index` that are set in `mask`. */
int msgSlotTest(int index, int mask) {
    return msgSlotFlags[index] & mask;
}

int msgWindowTestCurrentFlags(void *, int);

/* Nonzero while the window reports busy (flag mask 0x16). */
int msgWindowBusy(void) {
    return msgWindowTestCurrentFlags(msgWindow, 0x16);
}

int func_0027D378(unsigned char *);

int msgWindowHasMore(void) {
    return func_0027D378(msgWindow);
}

/* The current slot index (-1 when no slot is open). */
int msgSlotCurrent(void) {
    return msgSlotIndex;
}

INCLUDE_ASM("asm/main/nonmatchings/tu148", msgUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu148", msgUpdateTimed);

int msgWindowCurrentStatus(void *);
void msgQueueEntry(void *);

/* Close the window, the choice and the panel. */
void msgClose(void) {
    if ((msgWindowCurrentStatus(msgWindow) != 2) && (msgSlotTestCurrent(2) != 0)) {
        msgWindowActivateTalk();
    }
    msgWindowRequestFlush(msgWindow);
    msgQueueEntry(msgChoice);
    msgQueueEntry(msgPanel);
}

int msgWindowAdvance(void *);
void func_0027DE28(void *);

/* Begin a message: reset the window and the choice, open slot 0. */
void msgBegin(void) {
    msgWindowRunAll(msgWindow);
    func_0027DE28(msgChoice);
    msgSlotIndex = -1;
    msgWindowAdvance(msgWindow);
    msgSlotIndex += 1;
    msgGameFlagSet();
}

void msgWindowCloseForTalk(void) {
    msgWindowRunAll(msgWindow);
    msgSlotIndex = -1;
}

/* Clear the choice and stop its timer. */
void msgChoiceClear(void) {
    func_0027DE28(msgChoice);
    msgChoiceTimer = 0;
}

int msgWindowSetCurrentSlot(void *, int, int);

/* Set slot `slot` (1 to 10) of the current message record to `value`. */
void msgWindowSetSlot(int slot, int value) {
    msgWindowSetCurrentSlot(msgWindow, slot, value);
}

void msgWindowLayoutText(void *, int);

/* Set the text of the current message record. `text` is a string pointer
 * that every caller passes as an int. */
void msgWindowSetScriptText(int text) {
    msgWindowLayoutText(msgWindow, text);
}

void func_0027CC58(void *);
void msgWindowSetCurrentValue(void *, int);

/* Open the next window record with the text pointer `text` and raise the game flag. */
void msgWindowOpenText(int text) {
    func_0027CC58(msgWindow);
    msgWindowSetCurrentValue(msgWindow, text);
    msgGameFlagSet();
}


u32 msgWindowInUse(void) {
    if (msgWindowBusy() != 0) {
        return 1U;
    }
    return (u32) ~msgWindowGetFirstWord(msgWindow) >> 0x1F;
}

void msgWindowOpenNext(void) {
    func_0027CC58(msgWindow);
}

/* Set the window's value. */
void msgWindowSetValue(int value) {
    msgWindowSetCurrentValue(msgWindow, value);
}

void *msgWindowPollCurrent(void *);
void msgWindowMoveSelection(void *, int);

/* One pad record (PadData, stride 0x68). Layout as main/tu107's PadRecord:
 * direction_repeated 0x1000 is PAD_UP and 0x4000 PAD_DOWN (xgl_menu.c). */
typedef struct PadRecord {
    unsigned char unmodeled_00[0x2A];
    unsigned short buttons_2a;
    unsigned char unmodeled_2c[0x34 - 0x2C];
    unsigned short direction_repeated;
} PadRecord;

/* Poll the window: when it has finished and is ready, return the selected
 * index (as a pointer); CIRCLE (0x20) resets the window, UP and DOWN move the
 * selection by -1 and +1. Returns -1 otherwise. */
void *msgPollSelect(void) {
    void *selected;
    PadRecord *pad;

    if (msgWindowTestCurrentFlags(msgWindow, 0x14) == 0) {
        if ((msgWindowTestCurrentFlags(msgWindow, 8) == 0) && (msgWindowTestCurrentFlags(msgWindow, 2) != 0)) {
            selected = msgWindowPollCurrent(msgWindow);
            func_002512B0();
            return selected;
        }
        pad = (PadRecord *) PadData;
        if (pad->buttons_2a & 0x20) {
            msgWindowRunAll(msgWindow);
        }
        if (pad->direction_repeated & 0x1000) {
            msgWindowMoveSelection(msgWindow, -1);
        }
        if (pad->direction_repeated & 0x4000) {
            msgWindowMoveSelection(msgWindow, 1);
        }
    }
    return (void *) -1;
}

int msgChoiceShowSingle(void *, int);
void func_0027DEA0(void *, int, int);

/* Show the choice for `value`; its timer runs from 0x5A. */
void msgChoiceShow(int value) {
    msgChoiceTimer = 0x5A;
    msgChoiceShowSingle(msgChoice, value);
    func_0027DEA0(msgChoice, 0xA, 0x20);
}

void func_0027DD28(void *, int, int);

/* Show the choice for two values; its timer runs from 0x5A. */
void msgChoiceShowPair(int first, int second) {
    msgChoiceTimer = 0x5A;
    func_0027DD28(msgChoice, first, second);
    func_0027DEA0(msgChoice, 0xA, 0x20);
}

void msgChoiceSetIndex(void *, int);

/* Start the choice's timeline at `frame`, then the second step with `second`. */
void msgChoiceOpen(int frame, int second) {
    timecodeStartAtFrame(msgChoice, frame);
    msgChoiceSetIndex(msgChoice, second);
}

INCLUDE_ASM("asm/main/nonmatchings/tu148", msgChoicePoll);

void func_00251B18(void);
void timecodeStartAtHms(void *, int, int, int);

/* Open the panel, then show its default entry. */
void msgPanelOpen(void) {
    timecodeStartAtHms(msgPanel, 0, 0, 0);
    func_00251B18();
}

void timecodeSetHmsf(void *, int, int, int, int);

/* Set the panel's timecode: hours, minutes, seconds and frames. */
void msgPanelSetTimecode(int hours, int minutes, int seconds, int frames) {
    timecodeSetHmsf(msgPanel, hours, minutes, seconds, frames);
}

void msgPanelShow(int first, int second);

/* Show the panel entry 0x151 in mode 8. */
void func_00251B18(void) {
    msgPanelShow(0x151, 8);
}

/* Show a panel entry with `first` and `second`. */
void msgPanelShow(int first, int second) {
    func_0027DEA0(msgPanel, first, second);
}

/* Clear the panel. */
void msgPanelClear(void) {
    func_0027DE28(msgPanel);
}
