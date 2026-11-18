#ifndef INCLUDE_MAIN_TU233_H
#define INCLUDE_MAIN_TU233_H

#include "main/tu242.h"

/* Functions of src/main/tu233.c that other files call. */

void talkStashPrefix(MsgSlot *box);
void talkRestorePrefix(MsgSlot *box);
void talkCopySpeakerName(unsigned char *buffer, MsgSlot *box);

#endif /* INCLUDE_MAIN_TU233_H */
