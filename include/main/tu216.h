#ifndef INCLUDE_MAIN_TU216_H
#define INCLUDE_MAIN_TU216_H

/* Functions of src/main/tu216.c that other files call. */

/* Per-slot stored_value of the slots msgWindowAdvance and msgWindowOpenNextSlot open: MSG_SLOT_VALUE_BASE + MSG_SLOT_VALUE_STEP * slot index (meaning unconfirmed). */
#define MSG_SLOT_VALUE_BASE 0x0FFFFFCD
#define MSG_SLOT_VALUE_STEP 10

void msgWindowSetStartValue(int start_value);

#endif /* INCLUDE_MAIN_TU216_H */
