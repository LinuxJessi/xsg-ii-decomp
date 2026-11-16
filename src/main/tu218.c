#include "common.h"
#include "main/tu216.h"
#include "main/tu219.h"
#include "main/tu236.h"
#include "main/tu242.h"
#include "main/tu235.h"
#include "main/tu244.h"

/* Geometry of a newly opened window slot (x, y, width, height), forwarded to msgSlotSetGeometry. */
#define WINDOW_SLOT_X 0x96
#define WINDOW_SLOT_Y 0x9B
#define WINDOW_SLOT_WIDTH 0xE0
#define WINDOW_SLOT_HEIGHT 0xD6

void msgWindowOpen(MsgSlot *slot);

/* Opens the next message slot (at most five): initialises it as a window and sizes it. */
int msgWindowOpenNextSlot(MsgWindow *window)
{
    int index;
    MsgSlot *slot;

    index = window->current_index + 1;
    if (index >= MSG_SLOT_COUNT) {
        return 0;
    }
    window->current_index = index;
    slot = &window->slots[index];
    msgListSlotInit(slot);
    msgListSlotSetGeometry(slot, WINDOW_SLOT_X, WINDOW_SLOT_Y, WINDOW_SLOT_WIDTH, WINDOW_SLOT_HEIGHT);
    sizedRecordSetValue((struct SizedRecord *)slot, window->current_index * MSG_SLOT_VALUE_STEP + MSG_SLOT_VALUE_BASE);
    msgWindowOpen(slot);
    return 1;
}

/* Sets the text of the current slot, if one is open. */
void msgWindowSetCurrentValue(MsgWindow *window, int text)
{
    int index;
    MsgSlot *slot;

    index = window->current_index;
    slot = &window->slots[index];
    if (index >= 0) {
        msgWindowSetText(slot, text);
    }
}

/* Binds the item list of the current slot, if one is open: forwards the list source, cost table, count table and extra word (the arguments stay in their registers). */
void msgWindowBindCurrentItems(MsgWindow *window, int source, int costs, int *counts, int extra)
{
    int index;
    MsgSlot *slot;

    index = window->current_index;
    slot = &window->slots[index];
    if (index >= 0) {
        msgWindowBindItems(slot, source, costs, counts, extra);
    }
}
