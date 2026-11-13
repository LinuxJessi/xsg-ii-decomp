#include "common.h"
#include "main/tu097.h"
#include "main/tu098.h"

/* Text system sizes: eight text boxes, four slots per box stack, four cursor buffers, 0x18 pixels per line. */
#define TEXT_BOX_COUNT 8
#define TEXT_STACK_SLOTS 4
#define CURSOR_BUFFER_POOL_SIZE 4
#define TEXT_LINE_HEIGHT 0x18
/* textBoxUpdateInput turns the blink step back at these two values of its timer. */
#define TEXT_BLINK_LOW 0x21
#define TEXT_BLINK_HIGH 0x50

/*
 * A partial paged list: the item count at +0x08 and the items per page at +0x0C.
 */
typedef struct PagedCount {
    unsigned char unmodeled_00[8];
    int items;
    int perPage;
} PagedCount;

/* Returns the number of pages for the items when the list has both a count and a page size, else 0. */
int menuListPageCount(PagedCount *paged)
{
    if (paged->perPage == 0) {
        return 0;
    }
    if (paged->items == 0) {
        return 0;
    }
    return (unsigned int) (paged->items - 1) / (unsigned int) paged->perPage + 1;
}


typedef struct TextBoxEntryTail {
    unsigned char unmodeled_00[0x10];
    int color;
    int tailFlags;
    unsigned char byteD0;
    unsigned char unmodeled_19[0x03];
} TextBoxEntryTail;

typedef struct TextBoxEntry {
    int head;
    unsigned char unmodeled_04[0x08];
    int scrollOffset;
    unsigned char unmodeled_10[0x20];
    unsigned char sub30[0x14];
    int entryFlags;
    unsigned char unmodeled_48[0x08];
    int field50;
    unsigned char unmodeled_54[0x01];
    unsigned char byte55;
    unsigned char unmodeled_56[0x02];
    int x;
    int y;
    unsigned char unmodeled_60[0x58];
    TextBoxEntryTail tail;
} TextBoxEntry;

typedef struct TextBoxStack {
    int state;
    int shownFlags;
    TextBoxEntry entries[TEXT_STACK_SLOTS];
    int top;
    int pendingScroll;
} TextBoxStack;

void fontObjInitBlock(void *);
void fontObjSetExtra(void *, int);

/*
 * Initialises a TextBoxStack header: state 3, pending scroll and top -1, and the block and track of each of its four slots.
 */
void textBoxStackInit(TextBoxStack *stack)
{
    int i;

    if (stack != 0) {
        stack->state = 3;
        stack->top = -1;
        stack->pendingScroll = -1;
        i = 0;
        for (; i < TEXT_STACK_SLOTS; i++) {
            fontObjInitBlock(&stack->entries[i]);
            fontObjSetExtra(&stack->entries[i].x, 0xFFFFF0 + i);
        }
    }
}

/* A display track: position, one colour word at +0x70 and the rest of the block. */
typedef struct TextBoxTrack {
    int x;
    int y;
    unsigned char unmodeled_08[0x68];
    int color;
    unsigned char unmodeled_74[0x8];
} TextBoxTrack;

/* The first 0x5C bytes are a CursorBuffer (main/tu098.h); the calls to tu098 functions cast the record to it. */
typedef struct CursorBufferRecord {
    unsigned char unmodeled_00[0x1C];
    unsigned int state;
    unsigned char unmodeled_20[0x5C - 0x20];
    TextBoxTrack track;
} CursorBufferRecord;

/*
 * One 0xD8-byte entry of cursorBufferFlags, addressed through its own symbol (the
 * original code does not reach it as cursorBufferPool + 0x1C). Its first word is
 * the flags word that cursorBufferSetFlags and cursorBufferClearFlags update.
 */
typedef struct CursorBufferFlags {
    unsigned int flags;
    unsigned char unmodeled_04[0xD8 - 4];
} CursorBufferFlags;

typedef struct TextBox {
    int state;
    int lineCount;
    char *text;
    TextBoxStack stack;
} TextBox;

extern int cursorBufferCount;
extern int cursorBufferIndex;
extern CursorBufferRecord cursorBufferPool[];
extern CursorBufferFlags cursorBufferFlags[];
extern TextBox textBoxPool[];
/* Two-word callee: textBoxPush calls it with one argument and leaves the second word as is; textBoxSetTopText returns its result. */
void *fontObjSetText();
void fontObjSubAdvanceTracks(void *);
void *fontObjSetPosition(void *, int, int);
void *fontObjSetSize(void *, int, int);

/*
 * Pushes: advances the top index, capped at 3, and passes the new top entry to three hooks.
 * The text argument is passed by the caller and not read here.
 */
void textBoxPush(TextBoxStack *stack, char *text)
{
    int oldTop;

    if (stack != 0) {
        oldTop = stack->top;
        if (oldTop < TEXT_STACK_SLOTS - 1) {
            stack->top = oldTop + 1;
            fontObjSetText(&stack->entries[stack->top]);
            fontObjSubAdvanceTracks(&stack->entries[stack->top]);
            /* the entry's first 0x1C bytes are read as a track: fontTrackSetBoth writes its words */
            fontTrackSetBoth((struct FontTrack *) &stack->entries[stack->top], 0, 0);
        }
    }
}

/*
 * Sets the top entry's track to the point (x, y) through fontObjSetPosition, a plain wrapper of fontTrackSetBoth
 * (main/tu097), while the stack is not empty. textBoxShowPending passes (30, 200).
 */
void *textBoxSetTopPoint(TextBoxStack *stack, int x, int y)
{
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSetPosition(&stack->entries[stack->top].x, x, y);
        }
    }
    return result;
}

/* Passes the size (width, height) to fontObjSetSize for the top entry's track while the stack is not empty. */
void *textBoxSetTopSize(TextBoxStack *stack, int width, int height)
{
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSetSize(&stack->entries[stack->top].x, width, height);
        }
    }
    return result;
}

#include "shared.h"

/*
 * Sets the text of the top entry through fontObjSetText (main/tu097), which stores its second argument at
 * entry +0x40 and clears the entry's text state, while the stack is not empty. No caller in this TU.
 */
void *textBoxSetTopText(TextBoxStack *stack, int text) {
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSetText(&stack->entries[stack->top], text);
        }
    }
    return result;
}

void *fontObjSubStartOpen(void *entry);

/*
 * Activates the top entry: fontObjSubStartOpen calls fontObjStartOpen on its sub-object, which sets the active bit
 * that cursorBufferActivate also sets, while the stack is not empty.
 */
void *textBoxActivateTop(TextBoxStack *stack) {
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSubStartOpen(&stack->entries[stack->top]);
        }
    }
    return result;
}

/* Pops the top entry: the top index drops by one while the stack is not empty. */
void textBoxPop(TextBoxStack *stack) {
    if (stack != 0) {
        if (stack->top >= 0) {
            stack->top = stack->top - 1;
        }
    }
}

extern char D_00689FE8[];
void xglFontPrint(int x, int y, unsigned int color, const char *text);

/*
 * Prints the top entry of an unflagged stack at its track position, one 0x18 step per state count above it.
 */
void textBoxDrawMark(TextBoxStack *stack)
{
    if (!(stack->shownFlags & 2)) {
        xglFontPrint(stack->entries[stack->top].x + 0x1C8,
                     stack->entries[stack->top].y + stack->state * TEXT_LINE_HEIGHT - 8,
                     stack->entries[stack->top].tail.color + 1,
                     D_00689FE8);
    }
}

/*
 * Sets the stack's pending scroll count. textBoxUpdate lowers the entries' first words by one 0x18 step per
 * count and counts this word down to zero; textBoxReset and textBoxShowPending both store 0.
 */
void textBoxSetPendingScroll(TextBoxStack *stack, int pendingScroll) {
    stack->pendingScroll = pendingScroll;
}

void fontObjSubStartClose(void *entry);

/* Passes the top entry to fontObjSubStartClose (the open transition of its track). */
void textBoxStartTop(TextBoxStack *stack) {
    fontObjSubStartClose(&stack->entries[stack->top]);
}

extern unsigned short textOptionFlags[];
void fontObjDrawText(void *entry);

/*
 * Ticks the stack: walks the entries up to the top, runs the scroll step of the top entry for the pending
 * count, then starts or ticks the top entry according to its flags.
 */
void textBoxUpdate(TextBoxStack *stack)
{
    int i;

    if (stack == 0) {
        return;
    }
    for (i = 0; i <= stack->top; i++) {
        if ((unsigned int)stack->entries[i].field50 >= (unsigned int)(stack->state - 1)) {
            if (i == stack->top && stack->pendingScroll == -1) {
                stack->entries[i].scrollOffset = -(TEXT_LINE_HEIGHT * (stack->entries[i].field50 - stack->state) + TEXT_LINE_HEIGHT);
            }
        }
        fontObjDrawText(&stack->entries[i]);
    }
    if (stack->top < 0) {
        return;
    }
    if (stack->pendingScroll > 0) {
        do {
            stack->entries[stack->top].scrollOffset -= TEXT_LINE_HEIGHT;
            stack->pendingScroll--;
        } while (stack->pendingScroll > 0);
    }
    if ((stack->entries[stack->top].entryFlags & 0x10000) != 0) {
        if ((textOptionFlags[0] & 0x20) != 0 && (stack->shownFlags & 1) == 0) {
            fontObjSubStartClose(&stack->entries[stack->top]);
        }
        if ((stack->entries[stack->top].tail.tailFlags & 0x10000) == 0) {
            textBoxPop(stack);
        } else if (stack->entries[stack->top].tail.byteD0 != 2) {
            textBoxDrawMark(stack);
        }
    } else if ((stack->entries[stack->top].entryFlags & 0x20000) != 0) {
        textBoxDrawMark(stack);
        if ((textOptionFlags[0] & 0x20) != 0 || (stack->shownFlags & 1) != 0) {
            cursorBufferTick((CursorBuffer *) stack->entries[stack->top].sub30);
        }
    } else if ((textOptionFlags[0] & 0x20) != 0 && stack->entries[stack->top].tail.byteD0 == 0) {
        stack->entries[stack->top].byte55 = 100;
    }
}

/* The shared game-work block; only its flags word at +0x08 is touched by this TU. */
typedef struct GameWorkFlags {
    unsigned char unmodeled_00[8];
    unsigned int flags;
} GameWorkFlags;

extern GameWorkFlags gameWork;
void cursorBufferUpdateEntry(void *cursor);
/*
 * Updates every text box and draws the highlight bar of each box with a non-empty stack.
 */
void textBoxUpdateAll(void);
/*
 * Moves the selected line of each active box with the pad and updates the highlight blink counter.
 */
void textBoxUpdateInput(void);

void textSystemUpdate(void)
{
    int i;

    for (i = 0; i < CURSOR_BUFFER_POOL_SIZE; i++) {
        cursorBufferUpdateEntry(&cursorBufferPool[i]);
    }
    if (cursorBufferCount > 0) {
        cursorBufferHandlePad((CursorBuffer *) &cursorBufferPool[cursorBufferIndex]);
        if (!(cursorBufferPool[cursorBufferIndex].state & CURSOR_FLAG_ACTIVE)) {
            cursorBufferIndex = CURSOR_BUFFER_POOL_SIZE - 1;
            cursorBufferCount = cursorBufferCount - 1;
            if (!(cursorBufferPool[CURSOR_BUFFER_POOL_SIZE - 1].state & CURSOR_FLAG_ACTIVE)) {
                while (--cursorBufferIndex > 0) {
                    if (cursorBufferPool[cursorBufferIndex].state & CURSOR_FLAG_ACTIVE) {
                        break;
                    }
                }
            }
        }
        if (cursorBufferCount == 0) {
            gameWork.flags &= 0xFFFF7FFF;
        }
    }
    textBoxUpdateAll();
    textBoxUpdateInput();
}

#include "main/pad.h"

void cursorBufferSetText(void *, int);
void cursorBufferInitEntry(void *);
extern int D_0069D4B8;

void cursorBufferPush(int extra)
{
    CursorBufferRecord *buffer;
    int index;

    if (cursorBufferCount < 3) {
        for (index = cursorBufferIndex + 1; index < CURSOR_BUFFER_POOL_SIZE; index++) {
            buffer = &cursorBufferPool[index];
            if ((buffer->state & CURSOR_FLAG_ACTIVE) == 0) {
                cursorBufferDeactivate((CursorBuffer *) buffer);
                cursorBufferSetText(buffer, extra);
                /* the record's first 0x1C bytes hold the track words; the record is not typed as a track */
                fontTrackSetBoth((struct FontTrack *) buffer, 0, 0);
                cursorBufferActivate((CursorBuffer *) buffer);
                cursorBufferIndex = index;
                cursorBufferCount = cursorBufferCount + 1;
                return;
            }
        }
    }
}


/*
 * Resets one TextBox: clears its state word and the head word of its first entry (the text pointer
 * word at +0x08 of the box), then reinitialises the stack that starts at +0x0C.
 */
void textBoxReset(TextBox *box) {
    box->state = 0;
    box->text = 0;
    textBoxStackInit(&box->stack);
    textBoxSetPendingScroll(&box->stack, 0);
}

/*
 * Passes each block of textBoxPool to textBoxReset in turn. The end address
 * is compared as a signed word, as the original code does.
 */
void textBoxResetAll(void)
{
    TextBox *box;
    int boxEnd;

    box = textBoxPool;
    boxEnd = (int) (textBoxPool + TEXT_BOX_COUNT);
    do {
        textBoxReset(box);
        box++;
    } while ((int) box < boxEnd);
}

/* Resets the TextBox at index. */
void textBoxResetAt(int index) {
    textBoxReset(&textBoxPool[index]);
}

/*
 * Stores the text pointer of text box index (the word at +0x08 of its TextBox) while index is one of the eight
 * boxes. Returns 1 when the box was set, else 0.
 */
int textBoxSetText(int index, char *text)
{
    if ((unsigned int) index >= TEXT_BOX_COUNT) {
        return 0;
    }
    textBoxPool[index].text = text;
    return 1;
}

unsigned int strlen(const char *string);

/*
 * Counts the lines of a text: the literal backslash-n pairs in the first length-1 characters, plus one.
 */
int textBoxCountLines(const char *text)
{
    int length;
    int scanEnd;
    int count;
    int i;

    length = strlen(text);
    scanEnd = length - 1;
    count = 0;
    for (i = 0; i < scanEnd; i++) {
        if (text[i] == '\\' && text[i + 1] == 'n') {
            count++;
        }
    }
    return count + 1;
}

/* Height in pixels of a number of text lines: 0x18 per line. */
int textBoxLinesHeight(int lines) {
    return lines * TEXT_LINE_HEIGHT;
}

/*
 * Returns the widest line of a text times 10: the longest run of characters before each literal backslash-n.
 */
int textBoxMaxLineWidth(const char *text, int lineCount)
{
    int length;
    int i;
    int lineWidth;
    int maxWidth;

    length = strlen(text);
    lineWidth = 0;
    i = 0;
    maxWidth = 0;
    while (i < length) {
        if (text[i] == '\\') {
            if (text[i + 1] == 'n') {
                if (maxWidth < lineWidth) {
                    maxWidth = lineWidth;
                }
                lineWidth = 0;
                i++;
            }
        } else {
            lineWidth++;
        }
        i++;
    }
    if (maxWidth < lineWidth) {
        maxWidth = lineWidth;
    }
    return maxWidth * 10;
}

extern int textBoxPending;

/*
 * Queues the box at index as the pending one when its stack is empty (stackTop below 0) and it has text.
 * textBoxPending is stored on that path, keeping its old value when the box has no text.
 */
void textBoxQueue(int index)
{
    if ((unsigned int) index < TEXT_BOX_COUNT) {
        if (textBoxPool[index].stack.top < 0) {
            if (textBoxPool[index].text != 0) {
                textBoxPending = index;
            }
        }
    }
}

/* Returns 1 when any of the eight text boxes has a non-empty stack (stackTop not below 0), else 0. */
int textBoxAnyActive(void)
{
    int i;

    for (i = 0; i < TEXT_BOX_COUNT; i++) {
        if (textBoxPool[i].stack.top >= 0) {
            return 1;
        }
    }
    return 0;
}


int textBoxShowPending(void)
{
    int lineCount;
    int width;
    int index;

    if (textBoxPending >= 0) {
        textBoxStackInit(&textBoxPool[textBoxPending].stack);
        textBoxPush(&textBoxPool[textBoxPending].stack, textBoxPool[textBoxPending].text);
        ((void (*)(TextBoxStack *, int, int)) textBoxSetTopPoint)(&textBoxPool[textBoxPending].stack, 30, 200);
        {
            char *text = textBoxPool[textBoxPending].text;
            lineCount = textBoxCountLines(text);
        }
        textBoxPool[textBoxPending].lineCount = lineCount;
        width = textBoxMaxLineWidth(textBoxPool[textBoxPending].text, lineCount);
        index = textBoxPending;
        ((void (*)(TextBoxStack *, int, int)) textBoxSetTopSize)(&textBoxPool[index].stack, width, textBoxLinesHeight(textBoxPool[index].lineCount));
        textBoxActivateTop(&textBoxPool[textBoxPending].stack);
        textBoxSetPendingScroll(&textBoxPool[textBoxPending].stack, 0);
        {
            int shown = textBoxPending;
            textBoxPending = -1;
            textBoxPool[shown].state = 0;
            textBoxPool[shown].stack.shownFlags |= 1;
        }
        return 1;
    }
    return 0;
}

/*
 * Starts the window of text box index when it is valid and its stack is not empty, passing the box's stack to
 * textBoxStartTop. Returns no value of its own.
 */
int textBoxStartWindow(int index)
{
    if ((unsigned int) index < TEXT_BOX_COUNT) {
        if (textBoxPool[index].stack.top >= 0) {
            textBoxStartTop(&textBoxPool[index].stack);
        }
    }
}

extern int textHighlightAlpha;
int *fontObjGetOriginTrack(void *track);
int *fontObjGetSizeTrack(void *track);
void fontObjDrawQuad(int x0, int y0, int x1, int y1, int q_bits, int draw_bits);

/*
 * Updates every text box and, for each with a non-empty stack, draws the highlight bar behind its selected
 * line: the box's track position and size give the bar, one 0x18-pixel line high, tinted by the input counter.
 */
void textBoxUpdateAll(void)
{
    int i;
    int *position;
    int *size;

    for (i = 0; i < TEXT_BOX_COUNT; i++) {
        textBoxUpdate(&textBoxPool[i].stack);
        if (textBoxPool[i].stack.top >= 0) {
            position = fontObjGetOriginTrack(&textBoxPool[i].stack.entries[0].x);
            size = fontObjGetSizeTrack(&textBoxPool[i].stack.entries[0].x);
            fontObjDrawQuad(position[0], position[1] + textBoxPool[i].state * TEXT_LINE_HEIGHT, position[0] + size[0],
                            position[1] + textBoxPool[i].state * TEXT_LINE_HEIGHT + TEXT_LINE_HEIGHT, (textHighlightAlpha << 24) | 0x80, 0xFFFFFF);
        }
    }
}

extern PadRecord PadData;
extern int textHighlightAlphaStep;

/*
 * Moves the selected line of each active box with the pad: PAD_UP steps back and wraps to the last line,
 * PAD_DOWN steps forward and wraps to the first. Then it updates the two input counters.
 */
void textBoxUpdateInput(void)
{
    int i;
    int blinkTimer;

    for (i = 0; i < TEXT_BOX_COUNT; i++) {
        if (textBoxPool[i].stack.top >= 0) {
            if (textBoxPool[i].stack.entries[0].tail.byteD0 == 0) {
                if (PadData.repeated & PAD_UP) {
                    textBoxPool[i].state = textBoxPool[i].state - 1;
                    if (textBoxPool[i].state < 0) {
                        textBoxPool[i].state = textBoxPool[i].lineCount - 1;
                    }
                }
                if (PadData.repeated & PAD_DOWN) {
                    textBoxPool[i].state = textBoxPool[i].state + 1;
                    if (textBoxPool[i].state >= textBoxPool[i].lineCount) {
                        textBoxPool[i].state = 0;
                    }
                }
            }
        }
    }
    blinkTimer = textHighlightAlpha + textHighlightAlphaStep;
    textHighlightAlpha = blinkTimer;
    if ((unsigned int) blinkTimer < TEXT_BLINK_LOW) {
        textHighlightAlphaStep = 2;
        return;
    }
    if ((unsigned int) blinkTimer >= TEXT_BLINK_HIGH) {
        textHighlightAlphaStep = -2;
    }
}


int textBoxTryOpenWindow(int index)
{
    if (textBoxPool[index].stack.top < 0) {
        return -1;
    }
    if ((textOptionFlags[0] & 0x20) != 0) {
        if ((textBoxPool[index].stack.entries[0].entryFlags & 0x10000) != 0) {
            ((void (*)(int)) textBoxStartWindow)(index);
            if (cursorBufferCount > 0) {
                cursorBufferUpdateTracks((CursorBuffer *) &cursorBufferPool[cursorBufferIndex]);
            }
            return textBoxPool[index].state;
        }
        return -1;
    }
    return -1;
}

extern TextBox D_006A305C[];

u32 func_00210048(int index) {
    u32 inverted;

    inverted = ~D_006A305C[index].state;
    return inverted >> 0x1F;
}

void textSystemInit(void)
{
    int base;
    int track;
    int offset;
    int i;
    int extra;

    base = (int) cursorBufferPool;
    track = base + 0x5C;
    offset = 0;
    extra = 0xFFFFF0;
    for (i = 3; i >= 0; i--) {
        cursorBufferInitEntry((void *) (offset + base));
        fontObjSetExtra((void *) (offset + track), extra);
        offset += 0xD8;
        extra += 2;
    }
    cursorBufferCount = 0;
    cursorBufferIndex = 0;
    D_0069D4B8 = 0;
    textBoxResetAll();
}

/* Returns the current cursor buffer while the count is positive, else null. */
CursorBufferRecord *cursorBufferGetCurrent(void)
{
    if (cursorBufferCount > 0) {
        return &cursorBufferPool[cursorBufferIndex];
    }
    return 0;
}

int cursorBufferHasCurrent(void) {
    return cursorBufferCount > 0;
}

/* Sets the given bits in the current cursor buffer's state while the count is positive. */
void cursorBufferSetFlags(unsigned int bits)
{
    if (cursorBufferCount > 0) {
        cursorBufferFlags[cursorBufferIndex].flags |= bits;
    }
}

/* Clears the given bits in the current cursor buffer's state while the count is positive. */
void cursorBufferClearFlags(unsigned int mask)
{
    if (cursorBufferCount > 0) {
        cursorBufferFlags[cursorBufferIndex].flags &= ~mask;
    }
}

/* Passes the current cursor buffer to cursorBufferUpdateTracks while the count is positive. */
void cursorBufferUpdateCurrent(void)
{
    if (cursorBufferCount > 0) {
        cursorBufferUpdateTracks((CursorBuffer *) &cursorBufferPool[cursorBufferIndex]);
    }
}

/* Passes the current cursor buffer to cursorBufferDeactivate while the count is positive. */
void cursorBufferDeactivateCurrent(void)
{
    if (cursorBufferCount > 0) {
        cursorBufferDeactivate((CursorBuffer *) &cursorBufferPool[cursorBufferIndex]);
    }
}
