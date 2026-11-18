/*
 * OV01 original TU 52: 0x00aa4280..0x00aa4670 (5 functions)
 */
#include "common.h"
#include "ov01/tu005.h"
#include "ov01/tu051.h"

int actStockHasBoost(void);
void *objSlotAlloc(void);

/* Show the cursor: enabled, and when actStockHasBoost is clear, ready at the home position. */
void menuCursorShow(void)
{
    menuCursor->kind.cursor.visible = 1;
    if (actStockHasBoost() == 0) {
        menuCursor->moving = 1;
        menuCursor->slide.pos.target_x = MENU_CURSOR_HOME_X;
        menuCursor->slide.pos.target_y = MENU_CURSOR_HOME_Y;
    }
}

/* Hide the cursor: disabled and parked at (288, 454). */
void menuCursorHide(void)
{
    menuCursor->slide.pos.x = MENU_CURSOR_HOME_X;
    menuCursor->slide.pos.y = MENU_CURSOR_PARKED_Y;
    menuCursor->kind.cursor.visible = 0;
}

extern MenuObject *menuIconListObj;

/* Position of the icon list. */
#define MENU_ICON_LIST_X 8.0f
#define MENU_ICON_LIST_Y 312.0f

void menuIconListUpdate(MenuObject *list);
void menuIconTableDraw(MenuObject *list);

/* Creates the icon list object: no update step (menuIconListUpdate is empty), drawn
 * by menuIconTableDraw, with a cleared slide block at the list position. */
void menuIconListCreate(void)
{
    MenuObject *list;

    list = objSlotAlloc();
    if (list != 0) {
        list->update = menuIconListUpdate;
        list->draw = menuIconTableDraw;
        menuIconListObj = list;
        ov01MatrixBlockInit(&list->slide.block);
        list->slide.pos.x = MENU_ICON_LIST_X;
        list->slide.pos.y = MENU_ICON_LIST_Y;
        list->slide.pos.z = 0;
        list->moving = 0;
    }
}

/* Update step of the icon list: nothing to do. */
void menuIconListUpdate(MenuObject *list)
{
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu052", menuIconTableDraw);
