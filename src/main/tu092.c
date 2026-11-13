#include "common.h"
#include "shared.h"
#include "main/tu092.h"
#include "main/tu097.h"
#include "main/tu100.h"
#include "main/tu094.h"
#include "main/xgl_cd.h"
#include "main/xgl_font.h"
#include "main/pad.h"

char *strcpy(char *destination, const char *source);
extern unsigned char xglFontTextBuffer[];

/* The CD arena and buffers are handed out in whole 2048-byte CD sectors. */
#define CD_SECTOR_SIZE 0x800
#define CD_SECTOR_MASK (CD_SECTOR_SIZE - 1)

/* xglCdReadDoneCallback event and the packed tag kinds of xglCdReadFileAsync. */
enum CdEvent {
    CD_EVENT_DONE = 4
};

enum CdReadKind {
    CD_READ_FONT_SLOT = 1,
    CD_READ_BG_LAYER = 2,
    CD_READ_MENU_SKILL = 3,
    CD_READ_UMN_IMAGE = 4
};

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
    xglCdArenaUsed = (allocOffset + size + CD_SECTOR_MASK) & ~CD_SECTOR_MASK;
    return allocOffset;
}

/* xglCdArenaOpen(size, handle): a nonzero size is kept with the 64-bit handle; size 0 takes the size from
   GameResArenaReserve and sets the owned flag. */
int GameResArenaReserve(int size);
extern int xglCdArenaSize;
extern unsigned char xglCdArenaOwned;
extern long long xglCdArenaHandle;

void xglCdArenaOpen(int size, long long handle) {
    int reserved;

    if (size != 0) {
        xglCdArenaHandle = handle;
        xglCdArenaSize = size;
        xglCdArenaOwned = 0;
        xglCdArenaUsed = (size + CD_SECTOR_MASK) & ~CD_SECTOR_MASK;
    } else {
        if (handle == 0) {
            handle = 0x288000; /* default arena size, meaning not shown; kept as a literal */
        }
        reserved = GameResArenaReserve((int) handle);
        xglCdArenaUsed = (reserved + CD_SECTOR_MASK) & ~CD_SECTOR_MASK;
        xglCdArenaOwned = 1;
        xglCdArenaHandle = handle;
        xglCdArenaSize = reserved;
    }
}

void GameResArenaRelease();

/* xglCdArenaClose: when the arena is open, waits for the CD (xglCdWaitIdle), runs four cleanup calls, and clears
   the arena globals. */
void xglCdArenaClose(void) {
    if (xglCdArenaSize != 0) {
        xglCdWaitIdle();
        menuSkillModelReset();
        fontObjClearRunLists();
        bgLayerResetState();
        if (xglCdArenaOwned != 0) {
            GameResArenaRelease();
        }
    }
    xglCdArenaUsed = 0;
    xglCdArenaSize = 0;
    xglCdArenaOwned = 0;
    xglCdArenaHandle = 0;
}

/* xglCdArenaGetSize: returns xglCdArenaSize. */
int xglCdArenaGetSize(void) {
    return xglCdArenaSize;
}

/* xglCdBufferAlloc(size): reserves `size` bytes in the arena and points xglCdBufferBase and xglCdBufferCursor at
   them. */
extern int xglCdBufferBase;
extern int xglCdBufferCursor;

void xglCdBufferAlloc(long long size) {
    int base;

    base = xglCdSectorAlloc((int) size);
    xglCdBufferCursor = base;
    xglCdBufferBase = base;
}

/* xglCdBufferReserve(size): reserves `size` bytes at the CD buffer cursor and returns the old cursor; the cursor
   advances to the next 0x800-byte sector boundary. */
int xglCdBufferReserve(long long size) {
    int cursor = xglCdBufferCursor;

    xglCdBufferCursor = (cursor + (int) size + CD_SECTOR_MASK) & ~CD_SECTOR_MASK;
    return cursor;
}

void fontObjResetBufferSlots();

/* xglCdBufferRewind: waits for the CD (xglCdWaitIdle), runs two calls, moves the cursor back to the base, and
   returns the base. */
int xglCdBufferRewind(void) {
    int base;

    xglCdWaitIdle();
    fontObjResetBufferSlots();
    menuSkillModelTailReset();
    base = xglCdBufferBase;
    xglCdBufferCursor = base;
    return base;
}

/* xglCdBufferClear: zeroes xglCdBufferCursor and xglCdBufferBase. */
void xglCdBufferClear(void) {
    xglCdBufferCursor = 0;
    xglCdBufferBase = 0;
}

extern signed char pxyTickScaleDouble;
extern unsigned char pxyTickScaleDoubleSaved;

/* mode 1 saves pxyTickScaleDouble in pxyTickScaleDoubleSaved and sets it to 1 (doubled tick scale); mode 0 restores it from
   pxyTickScaleDoubleSaved. Other modes do nothing. */
void pxyTickScaleDoubleOverride(int mode) {
    unsigned char previousScale;

    if (mode != 0) {
        if (mode == 1) {
            previousScale = pxyTickScaleDouble;
            pxyTickScaleDouble = mode;
            pxyTickScaleDoubleSaved = previousScale;
        }
    } else {
        pxyTickScaleDouble = pxyTickScaleDoubleSaved;
    }
}

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
   dispatches the kind/sub byte pair of the packed tag (kind 1: fontObjMarkSlotLoaded, 2: bgLayerAdvance, 3: menuSkillModelReadDone,
   4: menuUmnImageDecode) and then decrements xglCdPendingReads. */
void fontObjMarkSlotLoaded(unsigned int sub);

void xglCdReadDoneCallback(int event, int value, unsigned int tag) {
    int kind;
    unsigned int sub;

    if (event < -2) {
        return;
    }
    if (event >= CD_EVENT_DONE) {
        if (event != CD_EVENT_DONE) {
            return;
        }
        kind = (tag >> 8) & 0xFF;
        sub = tag & 0xFF;
        switch (kind) {
        case CD_READ_FONT_SLOT:
            fontObjMarkSlotLoaded(sub);
            break;
        case CD_READ_BG_LAYER:
            bgLayerAdvance(sub);
            break;
        case CD_READ_MENU_SKILL:
            menuSkillModelReadDone(sub);
            break;
        case CD_READ_UMN_IMAGE:
            menuUmnImageDecode(sub);
            break;
        }
        xglCdPendingReads--;
    }
}

int xglCdReadFileAsync(const char *name, void *buffer, unsigned char kind, unsigned char sub) {
    int result;

    result = 0;
    if (name == 0) {
        return result;
    }
    if (buffer == 0) {
        return result;
    }
    xglCdSetReadTag((kind << 8) + sub);
    result = xglCdReadFile(name, buffer, 1, (int) xglCdReadDoneCallback);
    if (result >= 0) {
        xglCdPendingReads++;
    }
    return result;
}

/* Pad predicates on PadData: 1 when the bit 0x20 (CIRCLE) of directionPressed is set, or 0x40 (CROSS) for the
   second, and no bit of mask 0xF00F is set in the halfword at +0x30 (not named yet). */
/* local view: pad.h's PadRecord (unmodeled_30 as a halfword) changes the code of these predicates, so the halfword view stays. */
typedef struct PadHalves {
    unsigned char unmodeled_00[0x30];
    unsigned short unmodeled_30;
    unsigned short directionPressed;
} PadHalves;

extern PadHalves PadData[];

int padCirclePressedAlone(void) {
    PadHalves *pad;

    pad = PadData;
    if ((pad->unmodeled_30 & 0xF00F) == 0 && (pad->directionPressed & PAD_CIRCLE) != 0) {
        return 1;
    }
    return 0;
}

int padCrossPressedAlone(void) {
    PadHalves *pad;

    pad = PadData;
    if ((pad->unmodeled_30 & 0xF00F) == 0 && (pad->directionPressed & PAD_CROSS) != 0) {
        return 1;
    }
    return 0;
}

void vibrationStepFade();
extern int xglTickCounter;

/* xglTickAdvance: advances the 16-bit xglTickCounter, then calls vibrationStepFade. */
void xglTickAdvance(void) {
    xglTickCounter = (xglTickCounter + 1) & 0xFFFF;
    vibrationStepFade();
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
    if ((newHours & 0xFF) >= 100U) {
        playTime->hours = 99;
        playTime->minutes = 59;
        playTime->seconds = 59;
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
void xglCallbackQueueRun(void) {
    int i;

    if (xglCallbackQueue[0] != 0) {
        i = 0;
        do {
            ((void (*)(void)) xglCallbackQueue[i])();
            i = i + 1;
        } while (xglCallbackQueue[i] != 0);
    }
    xglCallbackQueueAdd(0);
}
