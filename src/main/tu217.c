#include "common.h"
#include "main/tu217.h"
#include "main/tu242.h"
#include "main/tu228.h"

struct SlotBlock; /* defined in tu226.c; a MsgSlot starts with the block */

/* tu226 defines slotBlockSetSlot as void; int here keeps its result in v0 for the caller below. */
int slotBlockSetSlot(struct SlotBlock *block, int index, int value);
void slotBlockSetFirstSlot(struct SlotBlock *block, int value);

/* Forwards two values to the slot block of the current slot; without a current slot (index -1)
 * it returns the byte offset of that index, the product the compiler computes before the test. */
int msgWindowSetCurrentSlot(MsgWindow *window, int first, int second) {
    int current;
    int offset;
    MsgSlot *slot;

    current = window->current_index;
    offset = current * sizeof(MsgSlot);
    slot = &window->slots[current];
    if (current >= 0) {
        return slotBlockSetSlot((struct SlotBlock *) slot, first, second);
    }
    return offset;
}

/* Sets the first slot of the current record's slot block, if any. */
void msgWindowSetCurrentFirstSlot(MsgWindow *window, int value) {
    int current;
    MsgSlot *slot;

    current = window->current_index;
    slot = &window->slots[current]; /* address formed before the current >= 0 test, as in the original */
    if (current >= 0) {
        slotBlockSetFirstSlot((struct SlotBlock *) slot, value);
    }
}

/* Close step of the message window, called from msgClose (main/tu148). */
void msgWindowActivateTalk(void) {
    talkActivate();
}
