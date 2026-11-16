#include "common.h"
#include "main/tu215.h"
#include "main/tu219.h"
#include "main/tu225.h"
#include "main/tu235.h"
#include "main/tu240.h"
/* Grid step and origin used by msgWindowRunCurrentCell (the 20, 30 and 64 of its
 * argument arithmetic). */
#define DISPATCH_STEP_X 20
#define DISPATCH_STEP_Y 30
#define DISPATCH_ORIGIN 64


void msgWindowRunCurrentAt(MsgWindow *window, int x, int y)
{
    int current;
    MsgSlot *slot;

    current = window->current_index;
    if (current < 0) {
        return;
    }
    slot = &window->slots[current];
    if (slot->flags == 0) {
        return;
    }
    switch (slot->state) {
    case MSG_SLOT_STATE_TEXT:
        msgTextSlotFit(slot, x, y, slot->extent_x, slot->extent_y);
        break;

    case MSG_SLOT_STATE_WINDOW:
        msgListSlotSetGeometry(slot, x, y, slot->extent_x, slot->extent_y);
        break;

    case MSG_SLOT_STATE_TIMECODE:
        timecodeTickIfActive(slot, x, y, slot->extent_x, slot->extent_y);
        break;
    }
}

void msgWindowRunCurrentAnchored(MsgWindow *window, int x, int y)
{
    int current;
    MsgSlot *slot;

    current = window->current_index;
    if (current < 0) {
        return;
    }
    slot = &window->slots[current];
    if (slot->flags == 0) {
        return;
    }
    switch (slot->state) {
    case MSG_SLOT_STATE_TEXT:
        msgTextSlotFit(slot, slot->x, slot->y, x, y);
        break;

    case MSG_SLOT_STATE_WINDOW:
        msgListSlotSetGeometry(slot, slot->x, slot->y, x, y);
        break;

    case MSG_SLOT_STATE_TIMECODE:
        timecodeTickIfActive(slot, slot->x, slot->y, x, y);
        break;
    }
}

void msgWindowRunCurrentCell(MsgWindow *window, int x, int y)
{
    int current;
    MsgSlot *slot;

    current = window->current_index;
    if (current >= 0) {
        slot = &window->slots[current];
        msgTextSlotFit(slot, slot->x, slot->y,
                      x * DISPATCH_STEP_X + DISPATCH_ORIGIN,
                      y * DISPATCH_STEP_Y + DISPATCH_ORIGIN);
    }
}
