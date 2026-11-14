#include "common.h"
#include "shared.h"
#include "main/pad.h"
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
extern PadRecord PadData[];
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
    talkStateReset();
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
    talkStateReset();
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

void msgWindowSeekToWindowEnd(void *window);
int msgWindowRunCurrentValue(void *window, int value);
void func_0027D438(void *window);
void msgWindowUpdateCurrent(void *window);
int msgWindowCurrentStatus(void *window);

/* One step of the open message: CIRCLE finishes the window's text, shows the next page or closes the
 * slot; CROSS closes a slot that allows it (flag 0x40) while the window is not locked (flag 4). */
void msgUpdate(void)
{
    if (msgSlotCurrent() < 0) {
        return;
    }
    if (msgWindowCurrentStatus(msgWindow) == 2) {
        return;
    }
    if ((PadData[0].pressed & PAD_CIRCLE) && msgSlotTestCurrent(1) == 0) {
        if (msgWindowTestCurrentFlags(msgWindow, 0x10) == 1) {
            msgWindowSeekToWindowEnd(msgWindow);
        } else if (msgWindowHasMore()) {
            if (msgWindowBusy() == 0) {
                msgWindowRunCurrentValue(msgWindow, 1);
                func_0027D438(msgWindow);
            }
        } else if (msgWindowInUse() == 1 && msgWindowBusy() == 0) {
            func_002512B0();
            msgWindowUpdateCurrent(msgWindow);
            msgSlotIndex--;
        }
    }
    if (msgSlotTestCurrent(0x40) && msgWindowTestCurrentFlags(msgWindow, 4) == 0 && (PadData[0].pressed & PAD_CROSS)) {
        func_002512B0();
        msgWindowUpdateCurrent(msgWindow);
        msgSlotIndex--;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu148", msgUpdateTimed);

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
        pad = PadData;
        if (pad->pressed & PAD_CIRCLE) {
            msgWindowRunAll(msgWindow);
        }
        if (pad->directionRepeated & PAD_UP) {
            msgWindowMoveSelection(msgWindow, -1);
        }
        if (pad->directionRepeated & PAD_DOWN) {
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

int msgChoiceTestFlags(void *choice, int mask);
long long msgChoiceGetFrames(void *choice);
void func_0027D9D8(void *choice, int step);
void func_0027D9F0(void *choice, int step);

/* Polls the open choice: CIRCLE closes it and returns its frame count; left/right and up/down move the
 * selection. -1 while nothing is chosen or no choice is open. */
int msgChoicePoll(void)
{
    int frames;

    if (msgChoiceTestFlags(msgChoice, 8) == 0) {
        return -1;
    }
    if (msgChoiceTestFlags(msgChoice, 0x300) != 0) {
        return -1;
    }
    if (PadData[0].pressed & PAD_CIRCLE) {
        frames = msgChoiceGetFrames(msgChoice);
        func_0027DE28(msgChoice);
        msgWindowRunAll(msgWindow);
        msgGameFlagClear();
        return frames;
    }
    if (PadData[0].directionRepeated & PAD_LEFT) {
        func_0027D9D8(msgChoice, 1);
    }
    if (PadData[0].directionRepeated & PAD_RIGHT) {
        func_0027D9D8(msgChoice, -1);
    }
    if (PadData[0].directionRepeated & PAD_UP) {
        func_0027D9F0(msgChoice, 1);
    }
    if (PadData[0].directionRepeated & PAD_DOWN) {
        func_0027D9F0(msgChoice, -1);
    }
    return -1;
}

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
