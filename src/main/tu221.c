#include "common.h"

/*
 * One 0x98-byte record of the message window (main/tu148). The first word of
 * the window is the index of the current record; the record the functions
 * below address is the one that starts `current_index` records in: word
 * `current_index * 0x26` from the window base.
 */
typedef struct MsgEntryBody {
    unsigned char unmodeled_00[0x1c];
    int state;
} MsgEntryBody;

typedef struct MsgEntry {
    int current_index;
    unsigned char unmodeled_04[0x04];
    MsgEntryBody body;
    int value;
    unsigned char unmodeled_2c[0x6c];
} MsgEntry;

int interpAdvance(void *, int);
int msgWindowScrollBy(void *, int);

/* Runs the current record by its state; states 1 and 2 call a handler with value. */
int msgWindowRunCurrentValue(MsgEntry *window, int value)
{
    int current;
    int state;
    int result;
    MsgEntry *entry;
    MsgEntryBody *body;

    current = window->current_index;
    result = current * 4;
    if (current >= 0) {
        entry = (MsgEntry *) ((int *) window + current * 0x26);
        body = &entry->body;
        state = body->state;
        result = state < 3;
        if (state != 2) {
            if (result != 0) {
                result = 1;
                if (state == 1) {
                    result = interpAdvance(body, value);
                }
            }
        } else {
            result = msgWindowScrollBy(body, value);
        }
    }
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/tu221", func_0027D378);

/* Stores value in the current record (its value word) and returns that record. */
void *msgWindowStoreValue(MsgEntry *window, int value) {
    int current;
    void *result;

    current = window->current_index;
    result = (void *) (current * 4);
    if (current >= 0) {
        MsgEntry *entry = (MsgEntry *) ((int *) window + current * 0x26);
        entry->value = value;
        result = entry;
    }
    return result;
}
