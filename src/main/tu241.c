#include "common.h"
#include "main/tu234.h"
#include "main/tu240.h"
#include "main/tu241.h"
#include "main/tu242.h"
#include "main/tu243.h"

/* Time units of the timecode object (the TIMECODE_* slot state, flags and phase codes are in main/tu240.h). */
#define FRAMES_PER_SECOND 30
#define FRAMES_PER_MINUTE (60 * FRAMES_PER_SECOND)
#define FRAMES_PER_HOUR (60 * FRAMES_PER_MINUTE)
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24            /* the hours field wraps at 24 */
#define TIMECODE_DIGIT_BUFFER 50    /* size of the digit string of timecodeAddToDigit */

void timecodeSetFrames(MsgSlot *slot, long long frames);

/* Prototypes of the callees whose headers do not declare them yet. */
int wrapIntRange(int, int, int);
long long timecodeAddToDigit(long long, int, int);
void numberToDigitsLsbFirst(char *out, long long value, int digits);
void digitsLsbFirstToNumber(long long *out, char *text);
long long timecodeAddToField(long long, int, int);

/* While active: flags &= ~0x40. */
void timecodeClearFlag40(MsgSlot *slot) {
    if (slot->state == TIMECODE_STATE) {
        slot->flags &= ~TIMECODE_FLAG_PHASE;
    }
}

/* While active: elapsed_frames = frames. */
void timecodeSetFrames(MsgSlot *slot, long long frames) {
    if (slot->state == TIMECODE_STATE) {
        slot->u.timecode.elapsed_frames = frames;
    }
}

/* Returns elapsed_frames while active, else 0. */
long long timecodeGetFrames(MsgSlot *slot) {
    if (slot->state != TIMECODE_STATE) {
        return 0;
    }
    return slot->u.timecode.elapsed_frames;
}

/* Splits elapsed_frames into hours, minutes, seconds and frames (30 per second); main 0x00285970. The frame remainder is scaled by HUNDREDTHS_PER_FRAME (hundredths of a second) into *frames. */
void timecodeSplit(MsgSlot *slot, int *hours, int *minutes, int *seconds, int *frames) {
    long long remaining = slot->u.timecode.elapsed_frames;
    int hh, mm, ss, ff;
    if (slot->state == TIMECODE_STATE) {
        hh = __divdi3(remaining, FRAMES_PER_HOUR);
        remaining -= (long long)hh * FRAMES_PER_HOUR;
        mm = __divdi3(remaining, FRAMES_PER_MINUTE);
        remaining -= (long long)mm * FRAMES_PER_MINUTE;
        ss = __divdi3(remaining, FRAMES_PER_SECOND);
        remaining -= (long long)ss * FRAMES_PER_SECOND;
        ff = func_002A6A18(dpmul(fptodp(__floatdisf(remaining)), HUNDREDTHS_PER_FRAME));
        *hours = hh; *minutes = mm; *seconds = ss; *frames = ff;
    }
}

/* While active: stores value in phase and phase_copy. */
void timecodeSetPhase(MsgSlot *slot, int value) {
    if (slot->state == TIMECODE_STATE) {
        slot->u.timecode.phase = value;
        slot->u.timecode.phase_copy = value;
    }
}

/* Returns phase while active, else 0. */
int timecodeGetPhase(MsgSlot *slot) {
    if (slot->state != TIMECODE_STATE) {
        return 0;
    }
    return slot->u.timecode.phase;
}

/* Steps the phase code by delta (wrapped into the range of the mode), sets flag 0x100 and restarts the level ramp; main 0x00285B18. */
void timecodeStepPhase(MsgSlot *slot, int delta) {
    MsgSlotTimecode *step = &slot->u.timecode;
    int phase;

    if (slot->state == TIMECODE_STATE && !(slot->flags & TIMECODE_FLAG_RAMP)) {
        if (slot->flags & TIMECODE_FLAG_LEVEL) {
            return;
        }
        slot->flags |= TIMECODE_FLAG_LEVEL;
        phase = step->phase + delta;
        step->phase_copy = step->phase;
        step->phase = phase;
        if (step->mode == TIMECODE_MODE_HMS) {
            step->phase = wrapIntRange(phase, TIMECODE_PHASE_SECONDS, TIMECODE_PHASE_HOURS);
        } else if (step->mode == TIMECODE_MODE_MSF) {
            step->phase = wrapIntRange(phase, TIMECODE_PHASE_FRAMES, TIMECODE_PHASE_MINUTES);
        } else {
            step->phase = wrapIntRange(phase, 1, step->index);
        }
        step->level = 0;
        step->level_step = easeAngleStep(step->phase_copy * TIMECODE_PHASE_PIXELS, step->phase * TIMECODE_PHASE_PIXELS);
    }
}

/* Starts a timeline: flag 0x200 and ramp setup, then by mode: then timecodeAddToDigit (mode 0) or timecodeAddToField (mode 1 or 2); main 0x00285BF8. */
void timecodeStart(MsgSlot *slot, int param) {
    MsgSlotTimecode *step = &slot->u.timecode;

    if (slot->state == TIMECODE_STATE && !(slot->flags & TIMECODE_FLAG_RAMP)) {
        if (slot->flags & TIMECODE_FLAG_LEVEL) {
            return;
        }
        slot->flags |= TIMECODE_FLAG_RAMP;
        step->phase_start = step->phase;
        step->digit = valueDigit(step->elapsed_frames, step->phase);
        step->ramp = 0;
        step->ramp_step = easeAngleStep(0, param * TIMECODE_PHASE_PIXELS);
        step->span = param;
        switch (step->mode) {
        case 0:
            step->elapsed_frames = timecodeAddToDigit(step->elapsed_frames, step->phase, param);
            break;
        case 1:
        case 2:
            step->elapsed_frames = timecodeAddToField(step->elapsed_frames, step->phase, param);
            break;
        }
    }
}

/*
 * Adds amountIn to the decimal digit of value at place pos (1 = ones), wrapping that digit through 0..9; main 0x00285CD0.
 * The wrap bounds are the lower digits of value with the digit set to 9 (maxValue) and to 0 (minValue).
 */
/* A struct-wrapped buffer so that the whole-buffer copies below are 8-byte-aligned struct copies. */
typedef struct { char t[TIMECODE_DIGIT_BUFFER]; } TimecodeDigitBuf __attribute__((aligned(8)));
long long timecodeAddToDigit(long long value, int pos, int amountIn) {
    long long amount = amountIn;
    TimecodeDigitBuf nines, digits, zeros;
    long long maxValue;
    long long minValue;
    int last = pos - 1;
    int i;

    numberToDigitsLsbFirst(digits.t, value, last);
    nines = digits;
    nines.t[last] = '9';
    zeros = digits;
    zeros.t[last] = '0';
    digitsLsbFirstToNumber(&maxValue, nines.t);
    digitsLsbFirstToNumber(&minValue, zeros.t);
    for (i = 0; i < last; i++) {
        amount *= 10;
    }
    value += amount;
    if (maxValue < value || value < minValue) {
        value -= amount * 10;
    }
    return value;
}

/* Applies amount to one field of a frame count split into hours, minutes, seconds and frames (30 per second),
 * wrapping inside the field; phase 10 = frames, 11 = seconds, 12 = minutes, 13 = hours. Returns the new frame count, or 0 for another phase. */
long long timecodeAddToField(long long frames, int phase, int amount) {
    int fields[4];
    int *field;
    int limit;
    int total = frames;
    fields[3] = total / FRAMES_PER_HOUR;
    total -= (int)((long long)fields[3] * FRAMES_PER_HOUR);
    fields[2] = total / FRAMES_PER_MINUTE;
    total -= (int)((long long)fields[2] * FRAMES_PER_MINUTE);
    fields[1] = total / FRAMES_PER_SECOND;
    total -= (int)((long long)fields[1] * FRAMES_PER_SECOND);
    fields[0] = total;
    switch (phase) {
    case TIMECODE_PHASE_FRAMES: field = &fields[0]; limit = FRAMES_PER_SECOND; break;
    case TIMECODE_PHASE_SECONDS: field = &fields[1]; limit = SECONDS_PER_MINUTE; break;
    case TIMECODE_PHASE_MINUTES: field = &fields[2]; limit = MINUTES_PER_HOUR; break;
    case TIMECODE_PHASE_HOURS: field = &fields[3]; limit = HOURS_PER_DAY; break;
    default: return 0;
    }
    *field += amount;
    if (*field >= limit) {
        *field -= limit;
    }
    if (*field < 0) {
        *field += limit;
    }
    return (long long)fields[3] * FRAMES_PER_HOUR + (long long)fields[2] * FRAMES_PER_MINUTE + (long long)fields[1] * FRAMES_PER_SECOND + fields[0];
}

/* While active: elapsed_frames = hours * 108000 + minutes * 1800 + seconds * 30, then timecodeSetFrames. */
void timecodeSetHms(MsgSlot *slot, int hours, int minutes, int seconds) {
    long long frames = (long long)hours * FRAMES_PER_HOUR + (long long)minutes * FRAMES_PER_MINUTE + (long long)seconds * FRAMES_PER_SECOND;

    if (slot->state == TIMECODE_STATE) {
        timecodeSetFrames(slot, frames);
    }
}
