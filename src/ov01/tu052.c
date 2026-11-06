/*
 * OV01 original TU 52: 0x00aa4280..0x00aa4670 (5 functions)
 */
#include "common.h"

typedef struct MenuCursor MenuCursor;

/* State block addressed through menuCursor.  Only the fields these three
 * functions write are named; their roles are the names' only evidence. */
struct MenuCursor {
    unsigned char unmodeled_00[0x10];
    float origin_x;             /* +0x10 */
    float origin_y;             /* +0x14 */
    unsigned char unmodeled_18[0x58];
    float target_x;             /* +0x70 */
    float target_y;             /* +0x74 */
    unsigned char unmodeled_78[0x18];
    int ready;                  /* +0x90 */
    int enabled;                /* +0x94 */
};

int func_00A91750(void);
extern MenuCursor *menuCursor;

/* Show the cursor: enabled, and when func_00A91750 is clear, ready at the home position. */
void menuCursorShow(void)
{
    menuCursor->enabled = 1;
    if (func_00A91750() == 0) {
        menuCursor->ready = 1;
        menuCursor->target_x = 288.0f;
        menuCursor->target_y = 390.0f;
    }
}

/* Hide the cursor: disabled and parked at (288, 454). */
void menuCursorHide(void)
{
    menuCursor->origin_x = 288.0f;
    menuCursor->origin_y = 454.0f;
    menuCursor->enabled = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu052", menuIconListCreate);

void func_00AA43A0(void)
{
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu052", menuIconTableDraw);
