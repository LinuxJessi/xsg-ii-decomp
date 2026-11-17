#include "common.h"
#include "main/tu229.h"
#include "main/tu215.h"
#include "main/tu220.h"
#include "main/tu227.h"
#include "main/tu245.h"
#include "main/tu242.h"
#include "main/tu238.h"
#include "main/tu240.h"

void msgSlotItemTick(MsgSlot *slot);
int msgSlotDraw(MsgSlot *slot);

/* Runs the window's five slots: an active slot goes to msgSlotRun, an inactive current slot re-seeks the last active one. */
void msgWindowRunSlots(MsgWindow *window) {
    MsgSlot *slots = window->slots;
    int index;

    msgWindowCompact(window);
    for (index = 0; index < MSG_SLOT_COUNT; index++) {
        MsgSlot *slot = &slots[index];

        if (!(slot->flags & MSG_SLOT_ACTIVE)) {
            if (index == window->current_index) {
                msgWindowFindLastActive(window);
            }
        } else {
            msgSlotRun(slot);
        }
    }
}

/* Runs one active slot's update and draw pair, chosen by the slot's type. */
void msgSlotRun(MsgSlot *slot) {
    if (!(slot->flags & MSG_SLOT_ACTIVE)) {
        return;
    }
    switch (slot->state) {
    case MSG_SLOT_STATE_TEXT:
        interpUpdate(slot);
        interpDrawTextSlot(slot);
        break;
    case MSG_SLOT_STATE_WINDOW:
        msgWindowUpdate(slot);
        msgWindowDraw(slot);
        break;
    case MSG_SLOT_STATE_TIMECODE:
        timecodeUpdate(slot);
        timecodeSlotDraw(slot);
        break;
    case MSG_SLOT_STATE_ITEM:
        msgSlotItemTick(slot);
        msgSlotDraw(slot);
        break;
    }
}

void talkEnd(void) {
    msgWindowUnloadBotanXtx();
    talkEndScript();
}
