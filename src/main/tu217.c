#include "common.h"
#include "main/tu217.h"
#include "main/tu228.h"

/* One 0x98-byte record of the message window (main/tu148); word 0 of the
 * window is the index of the current record, -1 for none. */
typedef struct MsgEntry {
    int current_index;
    unsigned char unmodeled_04[4];
    unsigned char payload[0x90];
} MsgEntry;

int slotBlockSetSlot(void *, int, int);

/* Forwards two values to the payload of the current record; without a
 * current record it returns the byte offset of the (negative) index. */
int msgWindowSetCurrentSlot(MsgEntry *window, int first, int second) {
    int current;
    int offset;
    unsigned char *payload;

    current = window->current_index;
    offset = current * sizeof(MsgEntry);
    payload = (window + current)->payload;
    if (current >= 0) {
        return slotBlockSetSlot(payload, first, second);
    }
    return offset;
}

INCLUDE_ASM("asm/main/nonmatchings/tu217", func_0027CBF8);


/* Close step of the message window, called from msgClose (main/tu148). */
void msgWindowActivateTalk(void) {
    talkActivate();
}
