#include "common.h"

#include "main/tu215.h"
#include "main/tu239.h"
#include "main/tu242.h"

/* Header form (tu227.h: void interpAdvance(MsgSlot *, int)) does not compile here: "void value not ignored" at the two uses of the result. The original returns the v0 the callee leaves on its state 1 path, the angle step it just stored, so this prototype keeps int. */
int interpAdvance(void *, int);
/* Header form (tu238.h: void msgWindowScrollBy(struct MsgSlot *, int)) does not compile here: "void value not ignored" at the use of the result. The original returns the v0 the callee leaves on the list path (the slide step it just stored; the skipped path leaves v0 untouched), so this prototype keeps int. */
int msgWindowScrollBy(MsgSlot *, int);
int talkHasMoreLines(void *);

/* Runs the current record by its state; text and window slots call a handler with value. */
int msgWindowRunCurrentValue(MsgWindow *window, int value)
{
    int current;
    int state;
    int result;
    MsgSlot *slot;

    current = window->current_index;
    result = current * 4; /* computed before the current >= 0 test, as in the original */
    if (current >= 0) {
        slot = &window->slots[current];
        state = slot->state;
        result = state < MSG_SLOT_STATE_TIMECODE;
        if (state != MSG_SLOT_STATE_WINDOW) {
            if (result != 0) {
                result = 1;
                if (state == MSG_SLOT_STATE_TEXT) {
                    result = interpAdvance(slot, value);
                }
            }
        } else {
            result = msgWindowScrollBy(slot, value);
        }
    }
    return result;
}

/* Polls the current record: a text slot asks whether the talk has more lines, a window slot whether the window overflows. */
int msgWindowPollMoreText(MsgWindow *window) {
    int pending = 0;
    int current = window->current_index;
    MsgSlot *slot;
    int state;

    if (current < 0) {
        return 0;
    }
    slot = &window->slots[current];
    state = slot->state;
    if (state != MSG_SLOT_STATE_WINDOW) {
        if (state < MSG_SLOT_STATE_TIMECODE) {
            if (state == MSG_SLOT_STATE_TEXT) {
                pending = talkHasMoreLines(slot);
            }
        }
    } else {
        pending = msgWindowOverflows(slot);
    }
    return pending;
}

/* Stores value in the current slot (its init_word) and returns the record base. */
void *msgWindowStoreValue(MsgWindow *window, int value) {
    int current;
    void *result;

    current = window->current_index;
    result = (void *) (current * 4); /* computed before the current >= 0 test, as in the original */
    if (current >= 0) {
        MsgSlot *slot = &window->slots[current];
        slot->init_word = value;
        result = (char *) slot - 8; /* the record base, 8 bytes before the slot, as the original returns */
    }
    return result;
}
