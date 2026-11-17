#ifndef INCLUDE_MAIN_TU227_H
#define INCLUDE_MAIN_TU227_H

#include "shared.h"
#include "main/tu242.h"

/* Functions of src/main/tu227.c: the state 1 (text) slot (MsgSlot of tu242.h, body MsgSlotText). */

/* Flag bits of MsgSlot.flags of a text slot beyond the phase bits shared with MSG_SLOT_* (tu242.h). */
#define INTERP_OPEN_FRAMES 12   /* frames the opening phase (flag 0x4) lasts before the slot is open */
#define INTERP_ANGLE_END 180    /* degrees at which the cosine ease reaches its end value */

void interpUpdate(MsgSlot *slot);
int interpAdvanceCursor(MsgSlot *slot);
int interpDrawTextSlot(MsgSlot *slot);
void interpRewind(MsgSlot *slot);
void interpSetPending(MsgSlot *slot, int value);
void interpAdvance(MsgSlot *slot, int advance);
int interpStepCosine(MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU227_H */
