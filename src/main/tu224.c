#include "common.h"
#include "main/tu223.h"
#include "main/tu224.h"
#include "main/tu240.h"
#include "main/tu242.h"
#include "main/tu244.h"

/* Timecode helpers of tu241 and tu242. */
void timecodeSetPhase(MsgSlot *slot, int phase);
void timecodeSetFrames(MsgSlot *slot, long long frames);
void timecodeSetFramePair(MsgSlot *slot, long long first, long long second);

/* Time units, as in tu241.c. */

/* Setup of a timecode started by timecodeStartAtHms (values as tu241.c's TIMECODE_* names). */
#define TIMECODE_BOX_X 0x151       /* x, y, width and height passed to timecodeTickIfActive */
#define TIMECODE_BOX_Y 0x14
#define TIMECODE_BOX_WIDTH 0x96
#define TIMECODE_BOX_HEIGHT 0x23

/* Starts the timeline at hours/minutes/seconds (30 frames per second); returns 0 while the in-use bit (MSG_SLOT_ACTIVE) is set. */
int timecodeStartAtHms(MsgSlot *state, int hours, int minutes, int seconds) {
    long long frames = (long long)hours * FRAMES_PER_HOUR + (long long)minutes * FRAMES_PER_MINUTE + (long long)seconds * FRAMES_PER_SECOND;

    if (state->flags & MSG_SLOT_ACTIVE) {
        return 0;
    }
    if (!timecodeStartAtFrame(state, frames)) {
        return 0;
    }
    timecodeInit(state);
    timecodeTickIfActive(state, TIMECODE_BOX_X, TIMECODE_BOX_Y, TIMECODE_BOX_WIDTH, TIMECODE_BOX_HEIGHT);
    sizedRecordSetValue(state, SLOT_START_VALUE);
    timecodeSetFrames(state, frames);
    timecodeSetFlag04(state);
    timecodeSetMode(state, TIMECODE_MODE_MSF);
    timecodeSetEditable(state, 1);
    timecodeSetPhase(state, TIMECODE_PHASE_SECONDS);
    return 1;
}

/* Sets the frame count from hours/minutes/seconds/frames at 30 frames per second. */
void timecodeSetHmsf(MsgSlot *state, int hours, int minutes, int seconds, int frames) {
    timecodeSetFrames(state, (long long)hours * FRAMES_PER_HOUR + (long long)minutes * FRAMES_PER_MINUTE + (long long)seconds * FRAMES_PER_SECOND + frames);
}

/* Forwards its arguments to timecodeSetMode. */
void timecodeSetModeForward(MsgSlot *timecode, int mode) {
    timecodeSetMode(timecode, mode);
}

/* Stores two frame counts: the first from hours/minutes/seconds at 30 frames per second, the second from a 60 per second count. */
void timecodeSetFramePairFromHms(MsgSlot *state, int hours, int minutes, int seconds, int hours60, int minutes60, int seconds60) {
    timecodeSetFramePair(state, (long long)hours * FRAMES_PER_HOUR + (long long)minutes * FRAMES_PER_MINUTE + (long long)seconds * FRAMES_PER_SECOND, seconds60 + (long long)hours60 * SECONDS_PER_HOUR + (long long)minutes60 * SECONDS_PER_MINUTE);
}
