#include "common.h"

/* Message window object (the msgWindow window of main/tu148 and main/tu149). Only the fields these
 * functions touch are modelled. The window's list (list at +0x28, main/tu238) starts at +0x28, so the
 * word at +0x4C is also the list's capacity (written by msgWindowBindItems); see the notes of the proposal. */
typedef struct MsgWindow {
    unsigned char unmodeled_00[0x1C];
    int state;                       /* +0x1C: 2 = window active; every entry point tests it */
    unsigned char unmodeled_20[0x0C];
    int text;                        /* +0x2C: text pointer; textMaxLineWidth measures its width */
    unsigned char unmodeled_30[0x1C];
    int line_count;                  /* +0x4C: line count of the text (func_00282D50) */
} MsgWindow;

int func_00282D50(int text);
void msgWindowLayoutSize(MsgWindow *window);
void msgWindowCenter(MsgWindow *window);

/* Stores a new text for an active window, its line count, then sizes and centres the window. */
void msgWindowSetText(MsgWindow *window, int text)
{
    if (window->state == 2) {
        window->text = text;
        window->line_count = func_00282D50(text);
        msgWindowLayoutSize(window);
        msgWindowCenter(window);
    }
}

/* Binds the item list of the window (func_0027CD50 in main/tu218): ready flag, cost table, count table, total. */
INCLUDE_ASM("asm/main/nonmatchings/tu236", msgWindowBindItems);

/* Window size: height = min(line_count, 5) * 30 + 64, width = max(text width + 64, 100). */
INCLUDE_ASM("asm/main/nonmatchings/tu236", msgWindowLayoutSize);

/* List height and width: 0xFE or 0xA5 by whether the item count is nonzero, width 0x180. */
INCLUDE_ASM("asm/main/nonmatchings/tu236", msgWindowSizeList);

/* List position: x = 0x40, y = 0x79 or 0x51 by whether the item count is nonzero. */
INCLUDE_ASM("asm/main/nonmatchings/tu236", msgWindowPlaceList);
