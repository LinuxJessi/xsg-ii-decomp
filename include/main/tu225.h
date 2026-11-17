#ifndef INCLUDE_MAIN_TU225_H
#define INCLUDE_MAIN_TU225_H

#include "main/tu242.h"

/* Functions of src/main/tu225.c that other files call. */

/* Sets the geometry of a state 1 (text) slot and the text rows and wrap width that follow from it. */
void msgTextSlotFit(MsgSlot *slot, int x, int y, int width, int height);
/* Feeds one text input (a text pointer) into a state 1 (text) slot. */
void msgEntryFeedText(MsgSlot *slot, char *input);
/* Starts a type 1 text slot. */
void msgTextSlotStart(MsgSlot *slot);
/* Starts a choice slot; returns 0 when the slot was left alone, 1 otherwise. */
int msgItemSlotStart(MsgSlot *slot, char *name, int count);
int textRowsBetween(int start, int end);
/* Nonzero when any bit of mask is set in *flags. */
int msgChoiceTestFlags(int *flags, int mask);

#endif /* INCLUDE_MAIN_TU225_H */
