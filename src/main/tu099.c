#include "common.h"
#include "main/tu097.h"

INCLUDE_ASM("asm/main/nonmatchings/tu099", func_0020F230);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxStackInit);

/*
 * The 0x7C-byte track of a slot from +0x58 on (the animation track that
 * fontTrackSetBoth drives). Its address is passed to fontObjSetPosition and
 * fontObjSetSize; its contents are not accessed here.
 */
typedef struct TextBoxSlotTrack {
    unsigned char unmodeled_00[0x7C];
} TextBoxSlotTrack;

/* One 0xD4-byte slot: its track sits at +0x58. */
typedef struct TextBoxSlot {
    unsigned char unmodeled_00[0x58];
    TextBoxSlotTrack track;
} TextBoxSlot;

/*
 * The stack header of a TextBoxStack: an 8-byte prefix, four slots at +0x08
 * (stride 0xD4), and the top index at +0x358 (8 + 4 * 0xD4). The same bytes
 * as TextBoxStack, viewed through the slots.
 */
typedef struct TextBoxSlotTable {
    unsigned char unmodeled_00[8];
    TextBoxSlot slots[4];
    int top;
} TextBoxSlotTable;

/* One 0xD8-byte cursor buffer of cursorBufferPool; its state word sits at +0x1C. */
typedef struct CursorBufferRecord {
    unsigned char unmodeled_00[0xD8];
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

/*
 * One 0x36C-byte text box of textBoxPool: a state word at +0x00, the line
 * count at +0x04 (textBoxShowPending stores it there), the text pointer at +0x08
 * (textBoxSetText stores it), and a TextBoxStack from +0x0C. textBoxResetAll
 * visits eight of them.
 */
typedef struct TextBox {
    unsigned char unmodeled_00[0x36C];
} TextBox;

extern int cursorBufferCount;
extern int cursorBufferIndex;
extern CursorBufferRecord cursorBufferPool[];
extern CursorBufferFlags cursorBufferFlags[];
extern TextBox textBoxPool[];
void fontTrackSetBoth(void *, int, int);
/* Two-word callee: textBoxPush calls it with one argument and leaves the second word as is; textBoxSetTopText returns its result. */
void *fontObjSetText();
void fontObjSubAdvanceTracks(void *);
void *fontObjSetSize(void *, int, int);
void cursorBufferUpdateTracks(void *);
void cursorBufferDeactivate(void *);

/* Pushes: advances the top index, capped at 3, and passes the new top entry to three hooks. */
void textBoxPush(TextBoxSlotTable *stack)
{
    int top;

    if (stack != 0) {
        top = stack->top;
        if (top < 3) {
            stack->top = top + 1;
            fontObjSetText(&stack->slots[stack->top]);
            fontObjSubAdvanceTracks(&stack->slots[stack->top]);
            fontTrackSetBoth(&stack->slots[stack->top], 0, 0);
        }
    }
}

/*
 * Sets the top entry's track to the point (x, y) through fontObjSetPosition, a plain wrapper of fontTrackSetBoth
 * (main/tu097), while the stack is not empty. textBoxShowPending passes (30, 200).
 */
void *textBoxSetTopPoint(TextBoxSlotTable *stack, int x, int y)
{
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSetPosition(&stack->slots[stack->top].track, x, y);
        }
    }
    return result;
}

/* Passes the size (width, height) to fontObjSetSize for the top entry's track while the stack is not empty. */
void *textBoxSetTopSize(TextBoxSlotTable *stack, int width, int height)
{
    void *result;

    result = stack;
    if (stack != 0) {
        if (stack->top >= 0) {
            result = fontObjSetSize(&stack->slots[stack->top].track, width, height);
        }
    }
    return result;
}

#include "shared.h"

/* One 0xD4-byte stack entry; its first word is cleared when its text box resets. */
typedef struct TextBoxEntry {
    int head;
    unsigned char unmodeled_04[0xD0];
} TextBoxEntry;

/*
 * The 0x360-byte stack of a TextBox (from its +0x0C): a state word (textBoxStackInit sets it to the
 * depth limit 3, textBoxShowPending sets bit 0 when the box is shown), a four-entry stack indexed by top
 * (-1 when empty), and a tail.
 */
typedef struct TextBoxStack {
    int state;
    unsigned char unmodeled_04[0x4];
    TextBoxEntry entries[4];
    int top;
    unsigned char unmodeled_35C[0x10];
} TextBoxStack;

/*
 * The 0x360 bytes from offset 0x0C to the end of a TextBoxStack; textBoxSetPendingScroll stores its last word.
 * textBoxUpdate counts that word down while it moves the entries, so it is read as a pending scroll amount.
 */
typedef struct TextBoxStackTail {
    unsigned char unmodeled_00[0x35C];
    int pendingScroll;
} TextBoxStackTail;

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
 * Activates the top entry: fontObjSubStartOpen calls func_0020DCF0 on its sub-object, which sets the active bit
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

INCLUDE_ASM("asm/main/nonmatchings/tu099", func_0020F498);

/*
 * Sets the stack's pending scroll count. textBoxUpdate lowers the entries' first words by one 0x18 step per
 * count and counts this word down to zero; textBoxReset and textBoxShowPending both store 0.
 */
void textBoxSetPendingScroll(TextBoxStackTail *stackTail, int pendingScroll) {
    stackTail->pendingScroll = pendingScroll;
}

void fontObjSubStartClose(void *entry);

/* Passes the top entry to fontObjSubStartClose (the open transition of its track). */
void textBoxStartTop(TextBoxStack *stack) {
    fontObjSubStartClose(&stack->entries[stack->top]);
}

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu099", func_0020F7B0);

INCLUDE_ASM("asm/main/nonmatchings/tu099", cursorBufferPush);

void textBoxStackInit(TextBoxStackTail *stackTail);

/*
 * Resets one TextBox: clears its state word and the head word of its first entry (the text pointer
 * word at +0x08 of the box), then reinitialises the stack that starts at +0x0C.
 */
void textBoxReset(TextBoxStack *box) {
    TextBoxStackTail *boxStack;

    boxStack = (TextBoxStackTail *) box->entries[0].unmodeled_04;
    box->entries[0].head = 0;
    box->state = 0;
    textBoxStackInit(boxStack);
    textBoxSetPendingScroll(boxStack, 0);
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
    boxEnd = (int) (textBoxPool + 8);
    do {
        textBoxReset(box);
        box++;
    } while ((int) box < boxEnd);
}

/* Resets the TextBox at index. */
void textBoxResetAt(int index) {
    textBoxReset(&textBoxPool[index]);
}

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxSetText);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxCountLines);

/* Height in pixels of a number of text lines: 0x18 per line. */
int textBoxLinesHeight(int lines) {
    return lines * 0x18;
}

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxMaxLineWidth);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxQueue);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxAnyActive);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxShowPending);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxStartWindow);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxUpdateAll);

INCLUDE_ASM("asm/main/nonmatchings/tu099", textBoxUpdateInput);

INCLUDE_ASM("asm/main/nonmatchings/tu099", func_0020FF88);

extern TextBoxStack D_006A305C[];

u32 func_00210048(int index) {
    u32 inverted;

    inverted = ~D_006A305C[index].state;
    return inverted >> 0x1F;
}

INCLUDE_ASM("asm/main/nonmatchings/tu099", textSystemInit);

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
        cursorBufferUpdateTracks(&cursorBufferPool[cursorBufferIndex]);
    }
}

/* Passes the current cursor buffer to cursorBufferDeactivate while the count is positive. */
void cursorBufferDeactivateCurrent(void)
{
    if (cursorBufferCount > 0) {
        cursorBufferDeactivate(&cursorBufferPool[cursorBufferIndex]);
    }
}
