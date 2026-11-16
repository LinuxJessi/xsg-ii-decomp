#ifndef INCLUDE_MAIN_TU232_H
#define INCLUDE_MAIN_TU232_H

#include "main/tu242.h"

/* Functions of src/main/tu232.c. The talk text box is a MsgSlot (tu242.h): x/y/z are the slot origin and depth,
 * u.text.start the panel height test, u.text.reveal the text, u.text.speaker_name the name plate and
 * u.text.font the font record. talkGetBoxLayoutX reads u.timecode.computed_value (its caller draws a timecode slot). */

int talkGetBoxLayoutX(MsgSlot *box);
int talkGetBoxLayoutY(MsgSlot *box);
void talkDrawTextBox(MsgSlot *box);

#endif /* INCLUDE_MAIN_TU232_H */
