#include "common.h"
#include "main/tu206.h"
#include "main/tu234.h"
#include "main/tu239.h"
#include "main/tu245.h"

/* Message window update (msgWindowUpdate, called from main/tu215). Flag bits of MsgWindow.flags:
 * 0x2 closing, 0x4 opening (12 frames, then 0x48), 0x8 pulse (func_00284760), 0x20 slide,
 * 0x100 sweep of the cursor. The phase counter is the unmodelled word at +0x18. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowUpdate);

/* Pulse of the list highlight: bounces two words at list+0x28 and list+0x2C between 0x10 and 0x45. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", func_00284760);

#include "shared.h"

/* Item list of the message window, at MsgWindow +0x28. Only the members the functions of this TU read or
 * write are modelled. Offsets are relative to the list. Member names are the roles the code shows. */
typedef struct MsgItemList {
    int cursor;                     /* +0x00: selected item index, also the index into counts */
    u8 unmodeled_04[0x4];           /* +0x04: window text pointer (MsgWindow.text in main/tu236) */
    int scroll;                     /* +0x08: first visible row, clamped to [0, capacity - visible_rows] */
    int slide_pos;                  /* +0x0C: current pixel position of the slide */
    int slide_to;                   /* +0x10: target pixel position, scroll * 30 */
    int slide_from;                 /* +0x14: position the slide started from */
    int slide_angle;                /* +0x18: slide phase in degrees */
    int slide_step;                 /* +0x1C: degrees per frame */
    int visible_rows;               /* +0x20: rows shown at once; no scrolling below 5 */
    int capacity;                   /* +0x24: 40 entries in counts; shares its word with MsgWindow.line_count */
    u8 unmodeled_28[0x8];           /* +0x28: highlight pulse value and step (func_00284760) */
    int sweep_value;                /* +0x30: value of the current cosine sweep */
    int sweep_end;                  /* +0x34: sweep target, reached at 180 degrees */
    int sweep_begin;                /* +0x38: sweep start, value at 0 degrees */
    int sweep_angle;                /* +0x3C: sweep phase in degrees */
    int sweep_step;                 /* +0x40: degrees per frame */
    int list_active;                /* +0x44: set to 1 by the bind; enables the list update */
    int list_source;                /* +0x48: bind argument; zero keeps the cursor on the text lines */
    u8 unmodeled_4C[0x4];           /* +0x4C: cost table pointer (main/tu149 msgItemCost) */
    int *counts;                    /* +0x50: 40 item counts (main/tu149 msgItemCount) */
    u8 unmodeled_54[0x4];           /* +0x54: total pointer (main/tu149 &D_008886E0) */
    int item_count;                 /* +0x58: number of nonzero entries in counts */
} MsgItemList;

typedef struct MsgWindow {
    u32 flags;                      /* +0x00: see the flag bits at msgWindowUpdate */
    u8 unmodeled_04[0x18];          /* +0x04: text pointer is +0x2C, phase counter at +0x18 */
    int state;                      /* +0x1C: 2 = window active */
    u8 unmodeled_20[0x8];
    MsgItemList list;               /* +0x28 */
} MsgWindow;

f32 cosf(f32);
extern f32 msgRadiansPerDegree;
int func_0027E960();
int drawLineList();
void drawSellPanel(void *);
int func_00286F08();
void msgWindowSetEmptyLayout(MsgWindow *);
void msgWindowSkipEmptyItem(MsgWindow *window);
void msgWindowMoveCursor(MsgWindow *, int);
int msgWindowNearestFilledIndex();
int msgWindowSweepValue(MsgWindow *window);
int msgWindowNearestItemOffset(int *unused_table, int index);

/* Draws the window parts for its phase flags; the list is drawn as text rows (drawLineList or drawSellPanel). */
void msgWindowDraw(MsgWindow *window)
{
    if (window->state == 2)
    {
        if (window->flags & 4)
        {
            func_0027E960(window);
        }
        if (window->flags & 2)
        {
            func_0027E960(window);
        }
        if (window->flags & 8)
        {
            do
            {
                if (window->list.list_active == 0)
                {
                    func_0027E960(window);
                    drawLineList(window);
                    break;
                }
                func_0027E960(window);
                drawSellPanel(window);
                msgWindowDrawListIcons(window);
            } while (0);
        }
    }
}

/* Closing phase: when the window is active, clears the opening, pulse, 0x40 and sweep bits and sets 0x2. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowClose);

/* Cursor index of an active window, 0 otherwise. */
int msgWindowGetItemIndex(MsgWindow *window)
{
    if (window->state != 2)
    {
        return 0;
    }
    return window->list.cursor;
}

/* Scrolls the list by delta rows and starts the 30-pixel slide that follows the scroll. */
void msgWindowScrollBy(MsgWindow *window, int delta)
{
    MsgItemList *list = &window->list;

    if (window->state == 2)
    {
        window->flags |= 0x20;
        list->slide_from = list->slide_pos;
        list->scroll += delta;
        list->scroll = xglClampInt(list->scroll, 0, list->capacity - list->visible_rows);
        list->slide_to = msgWindowRowPixels(list->scroll);
        list->slide_angle = 0;
        list->slide_step = func_00286F08(list->slide_from, list->slide_to);
    }
}

/* Slide step: advances slide_angle by slide_step; slide_pos moves from slide_from to slide_to on a cosine. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowSlideStep);

/* Sweep step: advances sweep_angle by sweep_step. At 180 degrees the value is sweep_end, then 1 is returned. */
int msgWindowSweepStep(MsgWindow *window)
{
    MsgItemList *list = &window->list;

    list->sweep_angle += list->sweep_step;
    if (list->sweep_angle >= 0xB4)
    {
        list->sweep_value = list->sweep_end;
        return 1;
    }
    list->sweep_value = msgWindowSweepValue(window);
    return 0;
}

/* Sweep value: cosine ease from sweep_begin (0 degrees) to sweep_end (180 degrees). */
int msgWindowSweepValue(MsgWindow *window)
{
    MsgItemList *list = &window->list;
    f32 amplitude;
    int midpoint;

    amplitude = (f32) -((list->sweep_end - list->sweep_begin) >> 1);
    midpoint = (list->sweep_end + list->sweep_begin) >> 1;
    return midpoint + (int) (amplitude * cosf((f32) list->sweep_angle * msgRadiansPerDegree));
}

/* Item count: the number of nonzero entries of the 40-entry count table. */
void msgWindowCountItems(MsgWindow *window)
{
    MsgItemList *list = &window->list;

    list->item_count = sellCountFilledSlots(list->counts);
}

/* Per-frame list update: keeps the cursor on a filled item, then clears the sweep and slide bits when the list is empty. */
void msgWindowUpdateList(MsgWindow *window)
{
    msgWindowSkipEmptyItem(window);
    msgWindowSetEmptyLayout(window);
    if (window->list.item_count == 0)
    {
        window->flags &= ~0x100;
        window->flags &= ~0x20;
    }
}

/* Moves the cursor off an empty item (count zero) to the nearest filled one. */
void msgWindowSkipEmptyItem(MsgWindow *window)
{
    MsgItemList *list = &window->list;

    if (list->list_active == 0 || list->list_source == 0)
    {
        return;
    }
    if (list->counts[list->cursor] == 0)
    {
        msgWindowMoveCursor(window, msgWindowNearestItemOffset(list->counts, list->cursor));
    }
}

/* Offset from index to the nearest filled item. */
int msgWindowNearestItemOffset(int *unused_table, int index)
{
    return msgWindowNearestFilledIndex() - index;
}

/* Index of the nearest filled entry: searches forward from the cursor first, then backward. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowNearestFilledIndex);

/* Scan of 4-byte entries from a pointer toward a bound: the first nonzero entry's address, or 0. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowFindFilledEntry);

/* Sets the cursor of an active window. */
void msgWindowSetItemIndex(MsgWindow *window, int index)
{
    if (window->state == 2)
    {
        window->list.cursor = index;
    }
}

/* First visible row (scroll), 0 when the window is not active. */
int msgWindowGetScroll(MsgWindow *window)
{
    if (window->state != 2)
    {
        return 0;
    }
    return window->list.scroll;
}

/* Scroll plus visible rows: the first row after the visible ones. */
int msgWindowGetScrollEnd(MsgWindow *window)
{
    if (window->state != 2)
    {
        return 0;
    }
    return window->list.scroll + window->list.visible_rows;
}

/* Visible rows of the list. */
int msgWindowGetVisibleRows(MsgWindow *window)
{
    if (window->state != 2)
    {
        return 0;
    }
    return window->list.visible_rows;
}

/* Moves the cursor by delta: the text-line path when list_active is 0, the skip-empty path otherwise. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowMoveCursor);

/* Text-line path: moves the cursor by delta over the lines of the text, wrapping, and starts the sweep. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowStepCursor);

/* Skip-empty path: moves the cursor by delta over filled items only, wrapping, and starts the sweep. */
INCLUDE_ASM("asm/main/nonmatchings/tu238", msgWindowStepCursorSkipEmpty);
