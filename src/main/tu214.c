#include "common.h"
#include "shared.h"
#include "main/tu207.h"
#include "main/tu214.h"
#include "main/tu215.h"
#include "main/tu229.h"
#include "main/tu234.h"
#include "main/tu242.h"
#include "main/tu245.h"

extern void *memset(void *destination, int value, unsigned int count);

/* Thin forwarder to recordPauseUpdates (tu207); no caller is recovered, so only the forwarding is evidenced. */
void func_0027C7D0(void) {
    recordPauseUpdates();
}

/* msgWindowReset: clears the window, sets current_index to -1 and runs three init helpers. */
void msgWindowReset(MsgWindow *window) {
    memset(window, 0, sizeof(MsgWindow));
    window->current_index = -1;
    talkStartScriptLoad();
    msgWindowLoadBotanXtx();
    textOutputReset();
}

/* msgWindowRequestFlush: the window (kept as void *, stored untyped in msgPendingWindow) becomes the pending window; msgWindowFlush is armed in slot 0. */
void msgWindowRequestFlush(void *window) {
    msgPendingWindow = window;
    msgFrameHooks[0] = msgWindowFlush;
}

/* msgQueueSlot: appends a slot pointer to the queue and arms msgWindowFlush in slot 0. */
void msgQueueSlot(MsgSlot *slot) {
    int index = msgSlotQueueCount;

    msgSlotQueue[index] = slot;
    msgFrameHooks[0] = msgWindowFlush;
    msgSlotQueueCount = index + 1;
}

/* msgWindowFlush: runs the pending window's slots, then the queued slots, then clears the queue. */
void msgWindowFlush(void) {
    int i;

    flushFontBatch();
    if (msgPendingWindow != 0) {
        msgWindowRunSlots(msgPendingWindow);
    }
    for (i = 0; i < msgSlotQueueCount; i++) {
        if (msgSlotQueue[i] != 0) {
            msgSlotRun(msgSlotQueue[i]);
        }
    }
    talkDeactivate();
    msgPendingWindow = 0;
    memset(msgSlotQueue, 0, sizeof(MsgSlot *) * MSG_SLOT_QUEUE_SIZE);
    msgSlotQueueCount = 0;
    msgFrameHooks[0] = 0;
}
