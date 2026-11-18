#ifndef INCLUDE_MAIN_TU223_H
#define INCLUDE_MAIN_TU223_H

#include "main/tu242.h"

/* Functions of src/main/tu223.c that other files call. */

/* Starts the timeline at a frame count (30 per second); returns 0 while the slot is in use. */
int timecodeStartAtFrame(MsgSlot *state, int frame);
/* Stores the choice index (forwards to timecodeSetIndex). */
void msgChoiceSetIndex(MsgSlot *slot, int index);

#endif /* INCLUDE_MAIN_TU223_H */
