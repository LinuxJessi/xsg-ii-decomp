#include "common.h"
#include "main/tu239.h"

/* Message window as msgWindowOverflows reads it. Only the state and the line count are named. */
struct MsgWindow {
    unsigned char unmodeled_00[0x1C];
    int state;
    unsigned char unmodeled_20[0x2C];
    int line_count;                 /* +0x4C: text line count, also read by main/tu237 */
};

/* Overflow test: 1 when the window is active and its text has six lines or more (layout clamps at five). */
int msgWindowOverflows(struct MsgWindow *window) {
    if (window->state != 2) {
        return 0;
    }
    if (window->line_count < 6) {
        return 0;
    }
    return 1;
}

/* Empty-list layout: when the item count is zero, height 0xA5, x 0x40, y 0x79 and width 0x180. */
INCLUDE_ASM("asm/main/nonmatchings/tu239", msgWindowSetEmptyLayout);

/* Row pitch of the message list: rows * 30 pixels (the step used by the list layout). */
int msgWindowRowPixels(int rows) {
    return rows * 0x1E;
}
