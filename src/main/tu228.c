#include "common.h"
#include "main/tu228.h"
#include "main/tu229.h"

/*
 * Talk-script scroll state. A TalkWork is one talk window: its state gates
 * every helper below, and the block walks the text buffer line by line.
 * The text lines are reached through the textSkipLines/textCountLinesBefore pair
 * (tu234): one turns a line index into a text position, the other counts
 * the lines up to a position.
 */
typedef struct TalkScrollBlock {
    int text_handle;                       /* text buffer the lines are read from */
    int cursor;                            /* current text position */
    unsigned char unmodeled_08[0x0C];
    int first_line;                        /* first line of the visible window */
    unsigned char unmodeled_18[0x14];
    int line_count;                        /* lines shown in the window */
    int setting;                           /* value stored by talkSetSetting */
    int line_limit;                        /* line index the window may not pass */
    unsigned char unmodeled_38[0x2C];
    unsigned int frame_counter;            /* ticks 0..TALK_FRAME_COUNTER_LIMIT - 1 */
} TalkScrollBlock;

typedef struct TalkWork {
    unsigned int flags;                    /* 0x10 is set by talkSetFlag10 */
    unsigned char unmodeled_04[0x18];
    int state;                             /* TALK_STATE_ACTIVE gates the helpers */
    unsigned char unmodeled_20[0x08];
    TalkScrollBlock block;
} TalkWork;

enum {
    TALK_STATE_ACTIVE = 1,
    TALK_FRAME_COUNTER_LIMIT = 16,
};

int textSkipLines(int text_handle, int line);
int textCountLinesBefore(int text_handle, int cursor);

/* Advances the window's frame counter, wrapping at TALK_FRAME_COUNTER_LIMIT. */
void talkAdvanceCounter(TalkWork *work) {
    TalkScrollBlock *block;
    unsigned int counter;

    block = &work->block;
    if (work->state == TALK_STATE_ACTIVE) {
        counter = block->frame_counter + 1;
        block->frame_counter = counter;
        if (counter >= TALK_FRAME_COUNTER_LIMIT) {
            block->frame_counter = 0;
        }
    }
}

/* Nonzero while the window's last line is still before line_limit. */
int talkHasMoreLines(TalkWork *work) {
    if (work->state != TALK_STATE_ACTIVE) {
        return 0;
    }
    return (work->block.first_line + work->block.line_count - 1) < work->block.line_limit;
}

void talkSetFlag10(TalkWork *work) {
    if (work->state == TALK_STATE_ACTIVE) {
        work->flags |= 0x10U;
    }
}

/* Moves the cursor by `amount` lines: position -> line index, add, back to a position. */
void talkScrollLines(TalkWork *work, int amount) {
    TalkScrollBlock *block;

    block = &work->block;
    if (work->state == TALK_STATE_ACTIVE) {
        block->cursor = textSkipLines(block->text_handle, textCountLinesBefore(block->text_handle, block->cursor) + amount);
    }
}

/* Moves the cursor to line_limit. */
void talkSeekToLimit(TalkWork *work) {
    TalkScrollBlock *block;

    block = &work->block;
    if (work->state == TALK_STATE_ACTIVE) {
        block->cursor = textSkipLines(block->text_handle, block->line_limit);
    }
}

/* Moves the cursor to the last line of the window. */
void talkSeekToWindowEnd(TalkWork *work) {
    TalkScrollBlock *block;

    block = &work->block;
    if (work->state == TALK_STATE_ACTIVE) {
        block->cursor = textSkipLines(block->text_handle, (block->first_line + block->line_count) - 1);
    }
}

/* Stores the mode value; the field is named setting until its consumers are read. */
void talkSetSetting(TalkWork *work, int setting) {
    if (work->state == TALK_STATE_ACTIVE) {
        work->block.setting = setting;
    }
}


/* Calls talkSetActive, which sets the talk-active flag talkActive. */
void talkActivate(void) {
    talkSetActive();
}

int xglFontCopyTextUnit(int, int, int);

/* Applies xglFontCopyTextUnit to the value twice (its meaning is not read yet). */
int func_0027E7A0(int value) {
    int result;
    int pass;

    result = value;
    for (pass = 0; pass < 2; pass++) {
        result = xglFontCopyTextUnit(0, result, 0);
    }
    return result;
}
