#ifndef INCLUDE_MAIN_TU230_H
#define INCLUDE_MAIN_TU230_H

/* Functions of src/main/tu230.c that other files call. */

#include "main/tu229.h"

void talkWindowFrameRect(int *left, int *top, int *bottom, int *right, MsgSlot *window, int mode, float scale);
unsigned int talkColorFadeAlpha(unsigned int color, float scale);
void talkWindowCursorPos(int *x, int *y, MsgSlot *window);

#endif /* INCLUDE_MAIN_TU230_H */
