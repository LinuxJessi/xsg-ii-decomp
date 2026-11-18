#ifndef INCLUDE_MAIN_TU220_H
#define INCLUDE_MAIN_TU220_H

#include "main/tu242.h"

/* Functions of src/main/tu220.c that other files call. */

int msgWindowGetFirstWord(int *word);
void msgWindowUpdateCurrent(MsgWindow *window);
void msgWindowFindLastActive(MsgWindow *window);
void msgWindowCompact(MsgWindow *window);
void msgWindowRunRecord(MsgWindow *window, int index);
/* msgWindowRunAll(MsgWindow *) is declared void * by its only other caller (main/tu148), so it stays out of this header. */

#endif /* INCLUDE_MAIN_TU220_H */
