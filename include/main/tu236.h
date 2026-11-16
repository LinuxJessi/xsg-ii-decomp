#ifndef INCLUDE_MAIN_TU236_H
#define INCLUDE_MAIN_TU236_H

/* Functions of src/main/tu236.c that other files use. */

#include "main/tu242.h"

/* Layout of the item list window (slot state 2) and of its empty-list variant. */
#define MSG_SLOT_STATE_LIST 2          /* slot state of an active list window; every entry point tests it */
#define MSG_LIST_X 0x40                /* x of the list window */
#define MSG_LIST_Y_FILLED 0x51         /* y when the list holds items */
#define MSG_LIST_Y_EMPTY 0x79          /* y when the list is empty */
#define MSG_LIST_WIDTH 0x180
#define MSG_LIST_HEIGHT_FILLED 0xFE
#define MSG_LIST_HEIGHT_EMPTY 0xA5
#define MSG_LIST_CAPACITY 40           /* item slots once the item list is bound */
#define MSG_LIST_ROW_PIXELS 30         /* height of one list row */
#define MSG_LIST_VISIBLE_ROWS 5        /* rows shown at most; the layout clamps there */
#define MSG_LIST_FRAME_PIXELS 64       /* window height beyond the rows, and width beyond the text */
#define MSG_LIST_MIN_WIDTH 100

void msgWindowSetText(MsgSlot *window, int text);
void msgWindowBindItems(MsgSlot *window, int source, int costs, int *counts, int extra);
void msgWindowLayoutSize(MsgSlot *window);
void msgWindowSizeList(MsgSlot *window);
void msgWindowPlaceList(MsgSlot *window);

#endif /* INCLUDE_MAIN_TU236_H */
