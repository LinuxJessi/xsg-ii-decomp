#ifndef INCLUDE_MAIN_TU241_H
#define INCLUDE_MAIN_TU241_H

#include "main/tu242.h"

/* Functions of src/main/tu241.c that other files call. */

/* The frame count of a timecode slot (30 per second), 0 unless the slot is a timecode slot. */
long long timecodeGetFrames(MsgSlot *slot);
/* Steps the phase code of a timecode slot by delta, wrapped into the range of its mode. */
void timecodeStepPhase(MsgSlot *slot, int delta);
/* Starts a timeline on a timecode slot (flag 0x200 and ramp setup, then by mode). */
void timecodeStart(MsgSlot *slot, int param);

#endif /* INCLUDE_MAIN_TU241_H */
