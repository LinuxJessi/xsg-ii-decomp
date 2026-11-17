#ifndef INCLUDE_MAIN_TU240_H
#define INCLUDE_MAIN_TU240_H

#include "shared.h"
#include "main/tu242.h"

/* Functions of src/main/tu240.c that other files call. */

#define TIMECODE_OPEN_FRAMES 12     /* frames of the opening and closing animation */
#define TIMECODE_ANGLE_END 180      /* the level and ramp run 0..180 degrees */
#define TIMECODE_COUNTER_WRAP 20
#define TIMECODE_COLOR_INIT 0xA8A8A8
/* Modes of the stepper (same numbering as the TextEntry kinds of main/tu234.c). */
/* Phase codes: the field being edited. */
#define TIMECODE_INDEX_MAX 9        /* index runs 1..9 */
#define TIMECODE_FIELD_PIXELS 75    /* pixels per field step in modes 1 and 2 */
#define TIMECODE_DURATION_BASE 0x54 /* mode 0 duration: base plus TIMECODE_PHASE_PIXELS per index step */
#define TIMECODE_DURATION_MIN 0xAA
#define TIMECODE_DURATION_TIME 150  /* duration of modes 1 and 2 */

/* Each function takes a timecode slot (MsgSlot of tu242.h, state 3; its body is MsgSlotTimecode). */
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
void timecodeSetEditable(MsgSlot *slot, int value);
void timecodeSetMode(MsgSlot *slot, int mode);
void timecodeSetIndex(MsgSlot *slot, int index);
void timecodeDuration(MsgSlot *slot);
void timecodeComputeResults(MsgSlot *slot);
void timecodeSetFlag40(MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU240_H */
