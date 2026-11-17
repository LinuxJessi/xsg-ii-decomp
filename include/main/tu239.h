#ifndef INCLUDE_MAIN_TU239_H
#define INCLUDE_MAIN_TU239_H

/* Functions of src/main/tu239.c that other files call. */

#include "main/tu236.h"

#define MSG_WINDOW_OVERFLOW_LINES 6    /* text line count from which msgWindowOverflows reports an overflow */

int msgWindowOverflows(MsgSlot *window);
void msgWindowSetEmptyLayout(MsgSlot *window);
int msgWindowRowPixels(int rows);

#endif /* INCLUDE_MAIN_TU239_H */
