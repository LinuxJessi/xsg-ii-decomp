#include "common.h"
#include "main/tu216.h"

/* Stores the start value that msgWindowAdvance passes to the record starter
 * (msgWindowStartValue; main/tu148 msgReset sets it to 1). */
extern int msgWindowStartValue;

void msgWindowSetStartValue(int start_value) {
    msgWindowStartValue = start_value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu216", msgWindowAdvance);

/* One 0x98-byte record of the message window (main/tu148, msgWindow).
 * Word 0 of the window is the index of the current record, -1 for none. */
typedef struct MsgEntry {
    int current_index;
    unsigned char unmodeled_04[4];
    unsigned char payload[0x90];
} MsgEntry;

void msgEntryFeedText();

void msgWindowLayoutText(MsgEntry *window, int value) {
    int current;

    current = window->current_index;
    if (current >= 0) {
        msgEntryFeedText(window[current].payload, value);
    }
}
