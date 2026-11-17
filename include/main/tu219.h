#ifndef INCLUDE_MAIN_TU219_H
#define INCLUDE_MAIN_TU219_H

/* Functions of src/main/tu219.c. */

#include "main/tu242.h"

void msgWindowRunCurrentAt(MsgWindow *window, int x, int y);
void msgWindowRunCurrentAnchored(MsgWindow *window, int x, int y);
void msgWindowRunCurrentCell(MsgWindow *window, int x, int y);

#endif /* INCLUDE_MAIN_TU219_H */
