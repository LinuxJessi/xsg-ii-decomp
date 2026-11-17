#include "common.h"
#include "main/tu216.h"
#include "main/tu227.h"
#include "main/tu244.h"
#include "main/tu225.h"
#include "main/tu242.h"

/* Stores the start value that msgWindowAdvance passes to the record starter
 * (msgWindowStartValue; main/tu148 msgReset sets it to 1). */
extern int msgWindowStartValue;

/* Geometry of a newly opened text slot, passed through msgTextSlotFit to msgSlotSetGeometry (x, y, width, height). */
#define TEXT_SLOT_X (-5)
#define TEXT_SLOT_Y 0x12C
#define TEXT_SLOT_WIDTH 0x208
#define TEXT_SLOT_HEIGHT 0x9A

void msgWindowSetStartValue(int start_value) {
    msgWindowStartValue = start_value;
}

struct SlotBlock;   /* defined by main/tu226 */
void slotBlockSetFlag04(struct SlotBlock *block);   /* sets flag 0x04 of the slot */

/* Opens the next message slot (at most five) as a text slot, sizes it and starts it; returns 1 on success, 0 when full. */
int msgWindowAdvance(MsgWindow *window) {
    int index;
    MsgSlot *slot;

    index = window->current_index + 1;
    if (index >= MSG_SLOT_COUNT) {
        return 0;
    }
    window->current_index = index;
    slot = &window->slots[index];
    msgTextSlotStart(slot);
    msgTextSlotFit(slot, TEXT_SLOT_X, TEXT_SLOT_Y, TEXT_SLOT_WIDTH, TEXT_SLOT_HEIGHT);
    sizedRecordSetValue(slot, window->current_index * MSG_SLOT_VALUE_STEP + MSG_SLOT_VALUE_BASE);
    slotBlockSetFlag04((struct SlotBlock *)slot);
    interpSetPending(slot, msgWindowStartValue);
    return 1;
}

/* Feeds the text input to the current slot of the message window, if there is one (current_index -1 for none). */
void msgWindowLayoutText(MsgWindow *window, char *text) {
    int current;

    current = window->current_index;
    if (current >= 0) {
        msgEntryFeedText(&window->slots[current], text);
    }
}
