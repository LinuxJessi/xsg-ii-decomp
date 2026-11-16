#include "common.h"
#include "main/tu229.h"
#include "main/tu234.h"
#include "main/tu240.h"
#include "main/tu242.h"
#include "main/tu244.h"

#include "shared.h"

/* Initialises the slot (state 3), sets phase and phase copy to 1, samples the cosine and sets the colour. */
void timecodeInit(MsgSlot *slot)
{
    MsgSlotTimecode *stepper = &slot->u.timecode;

    msgSlotInit(slot, TIMECODE_STATE);
    stepper->phase = 1;
    stepper->phase_copy = 1;
    stepper->computed_value = timecodeSampleCosine(slot);
    stepper->color = TIMECODE_COLOR_INIT;
}

/* Runs msgSlotSetGeometry with the four values while the object is active (state 3). */
void timecodeTickIfActive(MsgSlot *slot, int first_value, int second_value, int third_value, int fourth_value)
{
    if (slot->state == TIMECODE_STATE) {
        msgSlotSetGeometry(slot, first_value, second_value, third_value, fourth_value);
    }
}

/* While active: flags = (flags & ~closing) | opening (TIMECODE_FLAG_*). */
void timecodeSetFlag04(MsgSlot *slot)
{
    if (slot->state == TIMECODE_STATE) {
        slot->flags = (slot->flags & ~(u32) TIMECODE_FLAG_CLOSING) | TIMECODE_FLAG_OPENING;
    }
}

extern int gameLoopFlags[];
/* Bit 0 of the global flag word gameLoopFlags: while set, timecodeUpdate does nothing. */
#define SYS_FLAG_HOLD_UPDATES 0x1

/*
 * Per-frame update, main 0x00285160. Does nothing unless the object is active and gameLoopFlags bit 0 is clear.
 * The int return type (never set) keeps the final timecodeStepCounter call from becoming a tail jump.
 */
int timecodeUpdate(MsgSlot *slot)
{
    if (slot->state == TIMECODE_STATE && !(gameLoopFlags[0] & SYS_FLAG_HOLD_UPDATES)) {
        if (slot->flags & TIMECODE_FLAG_OPENING) {
            slot->reset_value += 1;
            if (slot->reset_value >= TIMECODE_OPEN_FRAMES) {
                slot->flags = (slot->flags & ~(u32) TIMECODE_FLAG_OPENING) | TIMECODE_FLAG_OPEN;
                switch (slot->u.timecode.value_b) {
                case 1:
                    break;
                case 2:
                    slot->flags |= TIMECODE_FLAG_PHASE;
                    break;
                }
            }
        }
        if (slot->flags & TIMECODE_FLAG_CLOSING) {
            slot->reset_value -= 1;
            if (slot->reset_value <= 0) {
                slot->flags = 0;
            }
        }
        if (slot->flags & TIMECODE_FLAG_ADVANCE) {
            timecodeAdvance(slot);
            if (slot->u.timecode.increment > 0) {
                if (slot->u.timecode.limit < slot->u.timecode.elapsed_frames) {
                    slot->flags &= ~(u32) TIMECODE_FLAG_ADVANCE;
                    slot->u.timecode.elapsed_frames = slot->u.timecode.limit;
                }
            } else if (slot->u.timecode.elapsed_frames < slot->u.timecode.limit) {
                slot->flags &= ~(u32) TIMECODE_FLAG_ADVANCE;
                slot->u.timecode.elapsed_frames = slot->u.timecode.limit;
            }
        }
        if (slot->flags & TIMECODE_FLAG_PHASE) {
            timecodeSelectPhase(slot);
        }
        if (slot->flags & TIMECODE_FLAG_RAMP) {
            if (timecodeRampStep(slot)) {
                slot->flags &= ~(u32) TIMECODE_FLAG_RAMP;
            }
        }
        if (slot->flags & TIMECODE_FLAG_LEVEL) {
            if (timecodeStepLevel(slot)) {
                slot->flags &= ~(u32) TIMECODE_FLAG_LEVEL;
            }
        }
        if (slot->flags & TIMECODE_FLAG_COUNTER) {
            timecodeStepCounter(slot);
        }
    }
}

/* Unconditional: elapsed_frames += increment. */
void timecodeAdvance(MsgSlot *slot)
{
    slot->u.timecode.elapsed_frames += slot->u.timecode.increment;
}

/* Forwards to timecodeSelectPhaseByMode, the phase selector. */
void timecodeSelectPhase(MsgSlot *slot)
{
    timecodeSelectPhaseByMode(slot);
}

/* Ramp step of ramp by ramp_step, clamped at 180; returns 1 at the clamp or when flag 0x200 is clear. */
int timecodeRampStep(MsgSlot *slot)
{
    if (!(slot->flags & TIMECODE_FLAG_RAMP)) {
        return 1;
    }
    slot->u.timecode.ramp += slot->u.timecode.ramp_step;
    if (slot->u.timecode.ramp >= TIMECODE_ANGLE_END + 1) {
        slot->u.timecode.ramp = TIMECODE_ANGLE_END;
        return 1;
    }
    return 0;
}

/*
 * Steps the level (angle) by level_step, clamps it at 180 and returns 1 when it
 * went past 180. Also stores the timecodeSampleCosine value in computed_value.
 */
int timecodeStepLevel(MsgSlot *slot)
{
    MsgSlotTimecode *stepper = &slot->u.timecode;
    int clamped = 0;

    stepper->level += stepper->level_step;
    if (stepper->level >= TIMECODE_ANGLE_END + 1) {
        clamped = 1;
        stepper->level = TIMECODE_ANGLE_END;
    }
    stepper->computed_value = timecodeSampleCosine(slot);
    return clamped;
}

/* Phase selector on mode and index; writes a phase code 10 to 13 into phase and phase_copy. */
void timecodeSelectPhaseByMode(MsgSlot *slot)
{
    int *phase = &slot->u.timecode.phase;
    int *phase_copy = &slot->u.timecode.phase_copy;

    switch (slot->u.timecode.mode) {
    case TIMECODE_MODE_NUMBER:
        if (*phase <= 0 || slot->u.timecode.index + 1 < *phase) {
            *phase = 1;
            *phase_copy = 1;
        }
        break;
    case TIMECODE_MODE_HMS:
        if ((unsigned)(*phase - TIMECODE_PHASE_SECONDS) < 3) {
            return;
        }
        *phase = TIMECODE_PHASE_HOURS;
        *phase_copy = TIMECODE_PHASE_HOURS;
        break;
    case TIMECODE_MODE_MSF:
        if ((unsigned)(*phase - TIMECODE_PHASE_FRAMES) < 3) {
            return;
        }
        *phase = TIMECODE_PHASE_MINUTES;
        *phase_copy = TIMECODE_PHASE_MINUTES;
        break;
    }
}

/* Wraps counter to 0 at 20. */
void timecodeStepCounter(MsgSlot *slot)
{
    MsgSlotTimecode *stepper = &slot->u.timecode;

    stepper->counter += 1;
    if (stepper->counter >= TIMECODE_COUNTER_WRAP) {
        stepper->counter = 0;
    }
}

/* Cosine value: the centre of the range from timecodeSampleRange with the half-width times cos(level). */
extern float D_0069B560;
extern float cosf(float angle);

int timecodeSampleCosine(MsgSlot *slot)
{
    MsgSlotTimecode *stepper = &slot->u.timecode;
    int low;
    int high;
    int centre;
    float amplitude;
    float angle;

    timecodeSampleRange(slot, &low, &high);
    angle = stepper->level * D_0069B560;
    amplitude = -((high - low) >> 1);
    centre = (high + low) >> 1;
    return centre + (int)(amplitude * cosf(angle));
}

/*
 * Low/high range for the cosine sample: mode 0 scales the distance of the two phase
 * codes from index by TIMECODE_PHASE_PIXELS; modes 1 and 2 use the D_0063D430 pair for the mode, scaled by TIMECODE_FIELD_PIXELS.
 */
/* Per-mode pair for modes 1 and 2: { base, phase offset }. */
extern int D_0063D430[2][2];

void timecodeSampleRange(MsgSlot *slot, int *low, int *high)
{
    MsgSlotTimecode *stepper = &slot->u.timecode;

    switch (stepper->mode) {
    case TIMECODE_MODE_NUMBER:
        *low = (stepper->index - stepper->phase_copy) * TIMECODE_PHASE_PIXELS;
        *high = (stepper->index - stepper->phase) * TIMECODE_PHASE_PIXELS;
        break;
    case TIMECODE_MODE_HMS:
        *low = (D_0063D430[0][0] - (stepper->phase_copy - D_0063D430[0][1])) * TIMECODE_FIELD_PIXELS;
        *high = (D_0063D430[0][0] - (stepper->phase - D_0063D430[0][1])) * TIMECODE_FIELD_PIXELS;
        break;
    case TIMECODE_MODE_MSF:
        *low = (D_0063D430[1][0] - (stepper->phase_copy - D_0063D430[1][1])) * TIMECODE_FIELD_PIXELS;
        *high = (D_0063D430[1][0] - (stepper->phase - D_0063D430[1][1])) * TIMECODE_FIELD_PIXELS;
        break;
    }
}

/*
 * Flag-driven dispatch while active: flags 0x4, 0x2 and 0x8 each run talkDrawFramesWithBox (mode 0) or
 * talkDrawFrameRects; flag 0x8 also draws the contents. talkBoxDrawContents (tu234) takes the slot as its TalkBoxPlace view.
 * The result is never used; int keeps the final call from becoming a tail jump.
 */
int timecodeSlotDraw(MsgSlot *slot)
{
    if (slot->state == TIMECODE_STATE) {
        if (slot->flags & TIMECODE_FLAG_OPENING) {
            if (slot->u.timecode.mode == TIMECODE_MODE_NUMBER) { talkDrawFramesWithBox(slot); } else { talkDrawFrameRects(slot); }
        }
        if (slot->flags & TIMECODE_FLAG_CLOSING) {
            if (slot->u.timecode.mode == TIMECODE_MODE_NUMBER) { talkDrawFramesWithBox(slot); } else { talkDrawFrameRects(slot); }
        }
        if (slot->flags & TIMECODE_FLAG_OPEN) {
            if (slot->u.timecode.mode == TIMECODE_MODE_NUMBER) { talkDrawFramesWithBox(slot); } else { talkDrawFrameRects(slot); }
            talkBoxDrawContents((struct TalkBoxPlace *) slot);
        }
    }
}

/* While active: clears flag bits 0x4, 0x8, 0x40, 0x80 and 0x800, then sets 0x2. */
void timecodeBeginClose(MsgSlot *slot)
{
    if (slot->state == TIMECODE_STATE) {
        slot->flags &= ~TIMECODE_FLAG_OPENING;
        slot->flags &= ~TIMECODE_FLAG_OPEN;
        slot->flags &= ~TIMECODE_FLAG_PHASE;
        slot->flags &= ~TIMECODE_FLAG_ADVANCE;
        slot->flags &= ~TIMECODE_FLAG_COUNTER;
        slot->flags |= TIMECODE_FLAG_CLOSING;
    }
}

/* While active: stores value in value_b. */
void timecodeSetValueB(MsgSlot *slot, int value)
{
    if (slot->state == TIMECODE_STATE) {
        slot->u.timecode.value_b = value;
    }
}

/*
 * While active: sets the mode, then recomputes the duration (timecodeDuration), the
 * phase (timecodeSelectPhaseByMode) and the computed value (timecodeSampleCosine).
 */
void timecodeSetMode(MsgSlot *slot, int mode)
{
    if (slot->state == TIMECODE_STATE) {
        slot->u.timecode.mode = mode;
        timecodeDuration(slot);
        timecodeSelectPhaseByMode(slot);
        slot->u.timecode.computed_value = timecodeSampleCosine(slot);
    }
}

/* While active and index in 1 to 9: stores the index, recomputes as timecodeSetMode does and stores computed_value. */
void timecodeSetIndex(MsgSlot *slot, int index)
{
    if (slot->state == TIMECODE_STATE && (unsigned)(index - 1) < TIMECODE_INDEX_MAX) {
        slot->u.timecode.index = index;
        timecodeDuration(slot);
        timecodeComputeResults(slot);
        timecodeSelectPhaseByMode(slot);
        slot->u.timecode.computed_value = timecodeSampleCosine(slot);
    }
}

/* Duration: TIMECODE_DURATION_TIME for modes 1 and 2; for mode 0, TIMECODE_DURATION_BASE + TIMECODE_PHASE_PIXELS per index step, at least TIMECODE_DURATION_MIN. */
void timecodeDuration(MsgSlot *slot)
{
    int n;

    switch (slot->u.timecode.mode) {
    case TIMECODE_MODE_NUMBER:
        n = slot->u.timecode.index - 1;
        slot->extent_x = n * TIMECODE_PHASE_PIXELS + TIMECODE_DURATION_BASE;
        if (slot->extent_x < TIMECODE_DURATION_MIN) {
            slot->extent_x = TIMECODE_DURATION_MIN;
        }
        break;
    case TIMECODE_MODE_HMS:
    case TIMECODE_MODE_MSF:
        slot->extent_x = TIMECODE_DURATION_TIME;
        break;
    }
}

/* Stores the results of sizedRecordCenterX and sizedRecordCenterY. */
void timecodeComputeResults(MsgSlot *slot)
{
    /* SizedRecord (tu244.c) is the shared view of the slot header (z, extent_x, extent_y) that tu216/218/224/225
     * and tu237 also pass to these helpers, so the casts stay until tu244 takes MsgSlot. Inlining the arithmetic
     * would drop the calls and change the rows. */
    slot->x = sizedRecordCenterX((struct SizedRecord *) slot);
    slot->y = sizedRecordCenterY((struct SizedRecord *) slot);
}

/* While active: flags |= 0x40. The opposite of timecodeClearFlag40 in tu241. */
void timecodeSetFlag40(MsgSlot *slot)
{
    if (slot->state == TIMECODE_STATE) {
        slot->flags |= TIMECODE_FLAG_PHASE;
    }
}
