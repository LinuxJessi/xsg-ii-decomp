#include "common.h"
#include "shared.h"
#include "main/tu092.h"
#include "main/tu097.h"
#include "main/xgl_cd.h"
#include "main/xgl_font.h"

char *strcpy(char *destination, const char *source);
extern unsigned char xglFontTextBuffer[];

/* xglFontGetTextWidth(text): copies the text into xglFontTextBuffer and returns the width xglFontGetStringWidth
   measures from two bytes before the buffer. */
int xglFontGetTextWidth(const char *text) {
    strcpy(xglFontTextBuffer, text);
    return xglFontGetStringWidth(xglFontTextBuffer - 2);
}

extern int xglCdArenaUsed;

/* xglCdSectorAlloc(size): bump allocator over xglCdArenaUsed; returns the old offset and advances it by `size`
   rounded up to 2048-byte CD sectors. */
int xglCdSectorAlloc(int size) {
    int allocOffset;

    allocOffset = xglCdArenaUsed;
    xglCdArenaUsed = (allocOffset + size + 0x7FF) & ~0x7FF;
    return allocOffset;
}

/* xglCdArenaOpen(size, handle): a nonzero size is kept with the 64-bit handle; size 0 takes the size from
   func_001E1830 and sets the owned flag. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCdArenaOpen);

void func_001E1A40();
void bgLayerResetState();
void func_00210C78();
extern int xglCdArenaSize;
extern unsigned char D_0069D480;
extern long long D_0069D488;

/* xglCdArenaClose: when the arena is open, waits for the CD (xglCdWaitIdle), runs four cleanup calls, and clears
   the arena globals. */
void xglCdArenaClose(void) {
    if (xglCdArenaSize != 0) {
        xglCdWaitIdle();
        func_00210C78();
        fontObjClearRunLists();
        bgLayerResetState();
        if (D_0069D480 != 0) {
            func_001E1A40();
        }
    }
    xglCdArenaUsed = 0;
    xglCdArenaSize = 0;
    D_0069D480 = 0;
    D_0069D488 = 0;
}

/* xglCdArenaGetSize: returns xglCdArenaSize. */
int xglCdArenaGetSize(void) {
    return xglCdArenaSize;
}

/* xglCdBufferAlloc(size): reserves `size` bytes in the arena and points xglCdBufferBase and xglCdBufferCursor at
   them. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCdBufferAlloc);

/* xglCdBufferReserve(size): advances xglCdBufferCursor by `size`, rounded to sectors. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCdBufferReserve);

void func_0020D430();
void func_00210CD8();
extern int xglCdBufferBase;
extern int xglCdBufferCursor;

/* xglCdBufferRewind: waits for the CD (xglCdWaitIdle), runs two calls, moves the cursor back to the base, and
   returns the base. */
int xglCdBufferRewind(void) {
    int base;

    xglCdWaitIdle();
    func_0020D430();
    func_00210CD8();
    base = xglCdBufferBase;
    xglCdBufferCursor = base;
    return base;
}

/* xglCdBufferClear: zeroes xglCdBufferCursor and xglCdBufferBase. */
void xglCdBufferClear(void) {
    xglCdBufferCursor = 0;
    xglCdBufferBase = 0;
}

/* Not named: saves or restores a byte flag (pxyTickScaleDouble with the saved copy D_0069D49C); its meaning is unknown. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", func_0020B7A8);

extern int xglCdPendingReads;

/* xglCdPendingReadsClear: zeroes xglCdPendingReads. */
void xglCdPendingReadsClear(void) {
    xglCdPendingReads = 0;
}


/* xglCdWaitIdle: loops while the CD reports busy (xglCdServiceIfPending) and while xglCdPendingReads is nonzero. */
void xglCdWaitIdle(void) {
    do {
        while (xglCdServiceIfPending() != 0) {
        }
    } while (xglCdPendingReads != 0);
}

/* xglCdReadDoneCallback(event, value...): completion callback of xglCdReadFileAsync. On CD_EVENT_DONE (4) it
   dispatches the kind/sub byte pair of the packed tag (kind 1: func_0020D0D0, 2: bgLayerAdvance, 3: func_00210D38,
   4: func_002106D0) and then decrements xglCdPendingReads. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCdReadDoneCallback);

/* xglCdReadFileAsync(name, buffer, kind, sub): stores the packed (kind, sub) tag through xglCdSetReadTag, starts
   xglCdReadFile with xglCdReadDoneCallback, and counts the read in xglCdPendingReads when it starts. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCdReadFileAsync);

/* Not named: pad predicates on PadData (func_0020B948 and func_0020B980 differ only in the 0x20 and 0x40 bits). */
INCLUDE_ASM("asm/main/nonmatchings/tu092", func_0020B948);

INCLUDE_ASM("asm/main/nonmatchings/tu092", func_0020B980);

void func_00208B80();
extern int xglTickCounter;

/* xglTickAdvance: advances the 16-bit xglTickCounter, then calls func_00208B80. */
void xglTickAdvance(void) {
    xglTickCounter = (xglTickCounter + 1) & 0xFFFF;
    func_00208B80();
    do { } while (0);
}

/* The play time shown by menuDrawPlayTime and the save slot screens: hours (+3, drawn without padding), minutes
   (+2) and seconds (+1), each 0..59 except hours; steps (+4) adds 24 hours per step after the first. */
struct PlayTime {
    unsigned char unmodeled_00;
    signed char seconds;
    signed char minutes;
    unsigned char hours;
    unsigned char steps;
};

/* playTimeClamp: adds the step hours to playTime->hours and clamps the total at 99:59:59 (hours 0x63). */
void playTimeClamp(struct PlayTime *playTime) {
    unsigned int newHours;
    unsigned int stepHours;

    stepHours = playTime->steps * 24 - 24;
    newHours = playTime->hours + stepHours;
    if (newHours >= 0x100U) {
        newHours = 0xFF;
    }
    playTime->hours = (unsigned char) newHours;
    if ((newHours & 0xFF) >= 0x64U) {
        playTime->hours = 0x63;
        playTime->minutes = 0x3B;
        playTime->seconds = 0x3B;
    }
}

void xglClockRead(void *clock_time);
unsigned int xglClockDayTime2UInt(void *clock_time);
extern int xglClockLastDayTime;

/* xglClockSampleDayTime: reads the clock and stores its day time in xglClockLastDayTime. */
void xglClockSampleDayTime(void) {
    unsigned int clock_time[4];

    xglClockRead(clock_time);
    xglClockLastDayTime = xglClockDayTime2UInt(clock_time);
}

struct ClockAccumulator {
    unsigned int unmodeled_00;
    int total;
};

extern struct ClockAccumulator *ptState[];

/* xglClockAccumulate: adds the day time elapsed since xglClockLastDayTime to xglClockAccumulators[0]->total. */
void xglClockAccumulate(void) {
    unsigned int clock_time[4];
    int newDayTime;
    int elapsed;

    xglClockRead(clock_time);
    elapsed = xglClockDayTime2UInt(clock_time) - xglClockLastDayTime;
    newDayTime = xglClockLastDayTime + elapsed;
    ptState[0]->total += elapsed;
    xglClockLastDayTime = newDayTime;
}

extern int xglCallbackQueueCount;
extern int xglCallbackQueue[];

/* xglCallbackQueueAdd(entry): appends a nonzero entry to xglCallbackQueue; zero empties the queue. */
void xglCallbackQueueAdd(int entry) {
    if (entry != 0) {
        xglCallbackQueue[xglCallbackQueueCount] = entry;
        xglCallbackQueueCount = xglCallbackQueueCount + 1;
        return;
    }
    xglCallbackQueueCount = 0;
    xglCallbackQueue[0] = 0;
}

/* xglCallbackQueueRun: calls each entry of xglCallbackQueue in order, up to the zero entry, then empties it. */
INCLUDE_ASM("asm/main/nonmatchings/tu092", xglCallbackQueueRun);
