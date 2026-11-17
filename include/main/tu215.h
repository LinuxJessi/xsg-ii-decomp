#ifndef INCLUDE_MAIN_TU215_H
#define INCLUDE_MAIN_TU215_H

#include "main/tu242.h"

/* Functions of src/main/tu215.c that other files call. */


/* Status codes msgWindowCurrentStatus (tu222) and msgSlotStatus (tu225) report for a slot. */
#define MSG_STATUS_NONE 0
#define MSG_STATUS_TEXT 1
#define MSG_STATUS_WINDOW 2
#define MSG_STATUS_TIMECODE_IDLE 3
#define MSG_STATUS_TIMECODE_RUNNING 4
#define MSG_STATUS_ITEM 5
/* A timecode slot (state 3) reports idle at mode 0 (MsgSlotTimecode.mode) and running for modes 1 up to (not including) this value. */
#define MSG_TIMECODE_SUB_RUNNING_END 3

void msgWindowRunSlots(MsgWindow *window);
void msgSlotRun(MsgSlot *slot);
/* Unloads the botan XTX, then ends the talk script. */
void talkEnd(void);

#endif /* INCLUDE_MAIN_TU215_H */
