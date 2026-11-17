#ifndef INCLUDE_MAIN_TU214_H
#define INCLUDE_MAIN_TU214_H

#include "main/tu242.h"

/* Functions of src/main/tu214.c that other files call. */

#define MSG_SLOT_QUEUE_SIZE MSG_SLOT_COUNT /* slots the queue holds, one per window slot */

/* Per-frame hook table; slot 0 holds msgWindowFlush while message slots are pending. */
extern void (*msgFrameHooks[])(void);
extern MsgWindow *msgPendingWindow;        /* window whose slots msgWindowFlush runs first */
extern MsgSlot *msgSlotQueue[];            /* MSG_SLOT_QUEUE_SIZE slots queued by msgQueueSlot */
extern int msgSlotQueueCount;              /* filled entries of msgSlotQueue */

/* msgWindowReset(MsgWindow *) is declared void * by its only caller (main/tu148), so it stays out of this header. */
void func_0027C7D0(void);
void msgWindowRequestFlush(void *window);
void msgQueueSlot(MsgSlot *slot);
void msgWindowFlush(void);

#endif /* INCLUDE_MAIN_TU214_H */
