#include "common.h"
#include "shared.h"
#include "main/tu207.h"
#include "main/tu214.h"
#include "main/tu229.h"
#include "main/tu242.h"
#include "main/tu245.h"

extern void *memset(void *destination, int value, unsigned int count);

/* The message window record (0x308 bytes) that msgWindowReset clears. currentSlot is the index of the slot being shown,
   -1 at reset. Five 0x98-byte slots start at +0x08 (see main/tu215 msgWindowRunSlots). */
typedef struct MsgWindow {
    int currentSlot;
    u8 unmodeled_04[0x304];
} MsgWindow;

void msgWindowFlush(void);
extern void *msgPendingWindow;
extern void (*D_0064587C[])(void);
extern int msgSlotQueue[];
extern int msgSlotQueueCount;

void func_0027C7D0(void) {
    recordPauseUpdates();
}

/* msgWindowReset: clears the window, sets currentSlot to -1 and runs three init helpers. */
void msgWindowReset(MsgWindow *window) {
    memset(window, 0, sizeof(MsgWindow));
    window->currentSlot = -1;
    talkStartScriptLoad();
    msgWindowLoadBotanXtx();
    textOutputReset();
}

/* msgWindowRequestFlush: the window (passed as an int) becomes the pending window; msgWindowFlush is armed in slot 0. */
void msgWindowRequestFlush(void *window) {
    msgPendingWindow = window;
    D_0064587C[0] = msgWindowFlush;
}

/* msgQueueSlot: appends a slot pointer to the queue and arms msgWindowFlush in slot 0. */
void msgQueueSlot(int slot) {
    int index = msgSlotQueueCount;

    msgSlotQueue[index] = slot;
    D_0064587C[0] = msgWindowFlush;
    msgSlotQueueCount = index + 1;
}

/* msgWindowFlush: runs the pending window's slots, then the queued slots, then clears the queue. */
INCLUDE_ASM("asm/main/nonmatchings/tu214", msgWindowFlush);
