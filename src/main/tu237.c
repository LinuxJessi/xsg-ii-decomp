#include "common.h"
#include "main/tu244.h"

/* Message window object as the centring functions use it (the window of main/tu236). Only the offsets
 * these functions touch are modelled. The window width at +0x10 is written by main/tu236 msgWindowLayoutSize
 * and read by sizedRecordCenterX (main/tu244); it sits in the unmodelled gap below. */
typedef struct MsgWindow {
    int flags;                      /* +0x00: 0x2 closing, 0x4 opening; msgWindowOpen swaps 0x2 for 0x4 */
    int pos_x;                      /* +0x04: centred x, from sizedRecordCenterX */
    int pos_y;                      /* +0x08: centred y, from msgWindowCenterY */
    unsigned char unmodeled_0C[8];
    int height;                     /* +0x14: lines * 30 + 64, halved for the centring */
    unsigned char unmodeled_18[4];
    int state;                      /* +0x1C: state code, 2 = active */
    unsigned char unmodeled_20[0x2C];
    int line_count;                 /* +0x4C: text line count, compared with 5 */
} MsgWindow;

int msgWindowCenterY(MsgWindow *window);

/* Centres the window: x comes from sizedRecordCenterX (0x100 - width / 2), y from msgWindowCenterY. */
void msgWindowCenter(MsgWindow *window)
{
    window->pos_x = sizedRecordCenterX(window);
    window->pos_y = msgWindowCenterY(window);
}

/* Vertical centre: 0xD2 - height / 2 from five lines up, 0xE0 - height / 2 below (sizedRecordCenterY has 0xE0). */
int msgWindowCenterY(MsgWindow *window)
{
    int half_height;

    half_height = window->height >> 1;
    if (window->line_count >= 5) {
        return 0xD2 - half_height;
    }
    return 0xE0 - half_height;
}

/* Opening phase: when the window is active, clears the closing bit 0x2 and sets the opening bit 0x4. */
void msgWindowOpen(MsgWindow *window)
{
    if (window->state == 2) {
        window->flags = (window->flags & ~2) | 4;
    }
}
