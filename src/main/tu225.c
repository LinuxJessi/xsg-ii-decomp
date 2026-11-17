#include "common.h"
#include "main/tu215.h"
#include "main/tu219.h"
#include "main/tu224.h"
#include "main/tu225.h"
#include "main/tu229.h"
#include "main/tu234.h"
#include "main/tu235.h"
#include "main/tu238.h"
#include "main/tu240.h"
#include "main/tu242.h"
#include "main/tu244.h"
#include "main/tu243.h"

/* Forwards to timecodeClearAdvance. */
void timecodeClearAdvanceForward(void *object) {
    timecodeClearAdvance(object);
}

void timecodeSplit(void);

/* Forwards to timecodeSplit (no arguments). */
void timecodeSplitForward(void) {
    timecodeSplit();
}

void timecodeSetStoredValue(void *object, int value) {
    timecodeSetColor(object, value);
}


void timecodeSetWideValue(void *record, int value) {
    timecodeStartCounter(record, value);
}

void msgChoiceShowSingle(MsgSlot *first, int second) {
    msgItemSlotStart(first, (char *)second, 0);
}

void msgSlotInitState4(MsgSlot *record);
void msgSlotSetItem(MsgSlot *slot, char *name, int count);
void msgSlotSetTimer(MsgSlot *object, int value);
void msgSlotSetFlag04(MsgSlot *object);
void msgSlotResetCounters(MsgSlot *record);
void msgSlotSetGeometryIfState4(MsgSlot *record, int x, int y, int extent_x, int extent_y);

/* Geometry (x, y, width, height) and timer of a (re)initialised item or choice slot. */
#define ITEM_SLOT_X 0
#define ITEM_SLOT_Y 30
#define ITEM_SLOT_WIDTH 200
#define ITEM_SLOT_HEIGHT 94
#define ITEM_SLOT_TIMER_FRAMES 0x78

/*
 * Starts a choice slot: unless the slot is already a state 4 slot with flag 0x1 set, it is
 * (re)initialised as state 4 with a fixed geometry; then the name is stored and the timer started.
 * Returns 0 when the slot was left alone, 1 otherwise.
 */
int msgItemSlotStart(MsgSlot *slot, char *name, int count) {
    int reinit;
    int flag_set;

    flag_set = msgChoiceTestFlags((int *)&slot->flags, MSG_SLOT_ACTIVE);
    if (slot->state != MSG_SLOT_STATE_ITEM && (flag_set & MSG_SLOT_ACTIVE)) {
        return 0;
    }
    reinit = msgChoiceTestFlags((int *)&slot->flags, MSG_SLOT_CLOSING) | !msgChoiceTestFlags((int *)&slot->flags, MSG_SLOT_ACTIVE);
    if (reinit) {
        msgSlotInitState4(slot);
        msgSlotSetGeometryIfState4(slot, ITEM_SLOT_X, ITEM_SLOT_Y, ITEM_SLOT_WIDTH, ITEM_SLOT_HEIGHT);
        sizedRecordSetValue(slot, MSG_SLOT_FRONT_Z);
    }
    msgSlotSetItem(slot, name, count);
    msgSlotSetTimer(slot, ITEM_SLOT_TIMER_FRAMES);
    if (reinit) {
        msgSlotSetFlag04(slot);
    } else {
        msgSlotResetCounters(slot);
    }
    return 1;
}

/* Per-state handler of a choice slot: a window slot is closed, a timecode slot runs its handler, an item slot gets flag 0x02 (closing). */
void msgChoiceCloseByState(MsgSlot *slot) {
    switch (slot->state) {
    case MSG_SLOT_STATE_WINDOW:
        msgWindowClose(slot);
        break;
    case MSG_SLOT_STATE_TIMECODE:
        timecodeBeginClose(slot);
        break;
    case MSG_SLOT_STATE_ITEM:
        msgSlotStartClosing(slot);
        slot = 0; /* dead store: keeps the call from becoming a tail call, as in the original */
        break;
    }
}


/* Positions a slot by its state: sets up the text entry, the window, the timecode or the item geometry. */
void msgSlotPlaceByState(MsgSlot *slot, int x, int y) {
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
    case MSG_SLOT_STATE_ITEM:
        msgSlotSetGeometryIfState4(slot, x, y, slot->extent_x, slot->extent_y);
        slot = 0; /* dead store: keeps the call from becoming a tail call, as in the original */
        break;
    }
}

/* Nonzero when any bit of mask is set in *flags; msgChoicePoll passes the choice object's first word. */
int msgChoiceTestFlags(int *flags, int mask) {
    return (*flags & mask) != 0;
}

/* Maps the state of an active slot to a MSG_STATUS_ code (same classification as msgWindowCurrentStatus, tu222); a timecode slot also looks at its mode. */
int msgSlotStatus(MsgSlot *slot) {
    int mode;

    if (!(slot->flags & MSG_SLOT_ACTIVE)) {
        return MSG_STATUS_NONE;
    }
    switch (slot->state) {
    case MSG_SLOT_STATE_TEXT:
        return MSG_STATUS_TEXT;
    case MSG_SLOT_STATE_WINDOW:
        return MSG_STATUS_WINDOW;
    case MSG_SLOT_STATE_TIMECODE:
        mode = slot->u.timecode.mode;
        if (mode == 0) {
            return MSG_STATUS_TIMECODE_IDLE;
        }
        /* nested ifs instead of one && condition: the original bytes branch twice */
        if (mode >= 0) {
            if (mode < MSG_TIMECODE_SUB_RUNNING_END) {
                return MSG_STATUS_TIMECODE_RUNNING;
            }
        }
        break;
    case MSG_SLOT_STATE_ITEM:
        return MSG_STATUS_ITEM;
    }
    return 0;
}

/* The two tag strings formatted into a type 1 text slot (role from msgTextSlotStart). */
extern const char textSlotStylePrefix[40];
char *textOutputAdvance(void);
void textFormatWrapped(void *record, int mapped);

/* Starts a type 1 text slot: two text output cursors, then two tag strings formatted into it. */
void msgTextSlotStart(MsgSlot *slot) {
    msgSlotInit(slot, MSG_SLOT_STATE_TEXT);
    slot->u.text.reveal = slot->u.text.start = textOutputAdvance();
    textFormatWrapped(slot, (int)textSlotStylePrefix);
    slot->u.text.reveal += 0x22; /* moves the revealed end on by 0x22 bytes after the first tag string (constant of the original) */
    textFormatWrapped(slot, (int)"$prop1;");
    slot->u.text.reveal += 7;
}

/* Pixels between the slot edge and the text rows, above and below them; and the height of one text row. */
#define TEXT_BUFFER_MARGIN 0x20
#define TEXT_BUFFER_STEP 30

/*
 * In state 1: applies the geometry, shows the text from line 1, counts the rows that fit between
 * TEXT_BUFFER_MARGIN pixels below the top and above the bottom, and sets the wrap width (textWrapWidth of the width).
 */
void msgTextSlotFit(MsgSlot *slot, int x, int y, int width, int height) {
    int state;
    int top;

    state = slot->state;
    if (state == MSG_SLOT_STATE_TEXT) {
        msgSlotSetGeometry(slot, x, y, width, height);
        top = slot->y;
        slot->u.text.first_line = state; /* state is MSG_SLOT_STATE_TEXT (1) here; the original stores that register */
        slot->u.text.visible_rows = textRowsBetween(top + TEXT_BUFFER_MARGIN, top + slot->extent_y - TEXT_BUFFER_MARGIN);
        slot->u.text.width_limit = textWrapWidth(width);
    }
}

/* Forwards to deltaPerStep with the fixed step TEXT_BUFFER_STEP (the same 0x1E step as tu235's msgListRowsBetween). */
int textRowsBetween(int start, int end) {
    return deltaPerStep(start, end, TEXT_BUFFER_STEP);
}

/*
 * In state 1: feeds one text input into the slot. A nonzero tag body is kept as the
 * speaker name, the input with its bracket tag skipped is formatted into the
 * slot, and the resulting line count is stored in line_count.
 */
void msgEntryFeedText(MsgSlot *record, char *input) {
    char *lookup;
    char *mapped;

    if (record->state == MSG_SLOT_STATE_TEXT) {
        lookup = textTagBody(input);
        if (lookup != 0) {
            record->u.text.speaker_name = lookup;
        }
        mapped = textSkipBracketTag(input);
        textFormatWrapped(record, (int)mapped);
        record->u.text.line_count = textLineCount(record);
    }
}

const char textSlotStylePrefix[40] = {
    11, 25, 2, 13, 2, 36, 120, 120,
    48, 44, 51, 48, 59, 36, 114, 117,
    98, 121, 122, 50, 51, 44, 50, 49,
    59, 36, 114, 117, 98, 121, 121, 45,
    50, 59, 0, 0, 0, 0, 0, 0
};
