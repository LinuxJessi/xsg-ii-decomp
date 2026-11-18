#ifndef INCLUDE_MAIN_TU231_H
#define INCLUDE_MAIN_TU231_H

/* Functions of src/main/tu231.c that other files call. */

struct MsgSlot;

void talkDrawFadeRect(float scale, int x0, int y0, int x1, int y1, unsigned int color);
void talkDrawSlotFrame(struct MsgSlot *slot);
void talkDrawSelectionBox(struct MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU231_H */
