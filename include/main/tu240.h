#ifndef INCLUDE_MAIN_TU240_H
#define INCLUDE_MAIN_TU240_H

#include "shared.h"
#include "main/tu242.h"

/* Functions of src/main/tu240.c that other files call. */

/* Slot state of a timecode object (MsgSlot.state). */
#define TIMECODE_STATE 3
/* Flag bits of MsgSlot.flags for a timecode slot. */
#define TIMECODE_FLAG_CLOSING 0x2
#define TIMECODE_FLAG_OPENING 0x4
#define TIMECODE_FLAG_OPEN 0x8
#define TIMECODE_FLAG_PHASE 0x40    /* timecodeUpdate runs the phase selector (the same bit as TALK_FLAG_BOX of tu229.h, which the box-drawing frame step tests) */
#define TIMECODE_FLAG_ADVANCE 0x80  /* elapsed_frames advances by increment each update */
#define TIMECODE_FLAG_LEVEL 0x100   /* level step in progress */
#define TIMECODE_FLAG_RAMP 0x200    /* ramp step in progress */
#define TIMECODE_FLAG_COUNTER 0x800 /* counter step runs each update */
#define TIMECODE_OPEN_FRAMES 12     /* frames of the opening and closing animation */
#define TIMECODE_ANGLE_END 180      /* the level and ramp run 0..180 degrees */
#define TIMECODE_COUNTER_WRAP 20
#define TIMECODE_COLOR_INIT 0xA8A8A8
/* Modes of the stepper (same numbering as the TextEntry kinds of main/tu234.c). */
#define TIMECODE_MODE_NUMBER 0
#define TIMECODE_MODE_HMS 1         /* hours, minutes, seconds: phases 11..13 */
#define TIMECODE_MODE_MSF 2         /* minutes, seconds, frames: phases 10..12 */
/* Phase codes: the field being edited. */
#define TIMECODE_PHASE_FRAMES 10
#define TIMECODE_PHASE_SECONDS 11
#define TIMECODE_PHASE_MINUTES 12
#define TIMECODE_PHASE_HOURS 13
#define TIMECODE_INDEX_MAX 9        /* index runs 1..9 */
#define TIMECODE_PHASE_PIXELS 20    /* pixels per phase step in mode 0 */
#define TIMECODE_FIELD_PIXELS 75    /* pixels per field step in modes 1 and 2 */
#define TIMECODE_DURATION_BASE 0x54 /* mode 0 duration: base plus TIMECODE_PHASE_PIXELS per index step */
#define TIMECODE_DURATION_MIN 0xAA
#define TIMECODE_DURATION_TIME 150  /* duration of modes 1 and 2 */

/* Each function takes a timecode slot (MsgSlot of tu242.h, state 3; its body is MsgSlotTimecode).
 * SizedRecord (tu244.h) is still a separate view of the same slot. */
void timecodeInit(MsgSlot *slot);
void timecodeTickIfActive(MsgSlot *slot, int first_value, int second_value, int third_value, int fourth_value);
void timecodeSetFlag04(MsgSlot *slot);
int timecodeUpdate(MsgSlot *slot);     /* int only to keep the last call from becoming a tail jump; the result is never set */
void timecodeAdvance(MsgSlot *slot);
void timecodeSelectPhase(MsgSlot *slot);
int timecodeRampStep(MsgSlot *slot);
int timecodeStepLevel(MsgSlot *slot);
void timecodeSelectPhaseByMode(MsgSlot *slot);
void timecodeStepCounter(MsgSlot *slot);
int timecodeSampleCosine(MsgSlot *slot);
void timecodeSampleRange(MsgSlot *slot, int *low, int *high);   /* low/high range for the cosine sample */
int timecodeSlotDraw(MsgSlot *slot);      /* flag-driven draw dispatch; the result is never used */
void timecodeBeginClose(MsgSlot *slot);
void timecodeSetValueB(MsgSlot *slot, int value);
void timecodeSetMode(MsgSlot *slot, int mode);
void timecodeSetIndex(MsgSlot *slot, int index);
void timecodeDuration(MsgSlot *slot);
void timecodeComputeResults(MsgSlot *slot);
void timecodeSetFlag40(MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU240_H */
